////////////////////////////////////////////////////////////////////////////////
/// @brief V8 action functions
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2004-2012 triAGENS GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is triAGENS GmbH, Cologne, Germany
///
/// @author Dr. Frank Celler
/// @author Copyright 2011-2012, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#include "v8-actions.h"

#include "Actions/actions.h"
#include "Basics/ReadLocker.h"
#include "Basics/StringUtils.h"
#include "Basics/WriteLocker.h"
#include "BasicsC/conversions.h"
#include "Logger/Logger.h"
#include "Rest/HttpRequest.h"
#include "Rest/HttpResponse.h"
#include "V8/v8-conv.h"
#include "V8/v8-utils.h"
#include "V8Server/v8-vocbase.h"

using namespace std;
using namespace triagens::basics;
using namespace triagens::rest;

// -----------------------------------------------------------------------------
// --SECTION--                                              forward declarations
// -----------------------------------------------------------------------------

static HttpResponse* ExecuteActionVocbase (TRI_vocbase_t* vocbase,
                                           void* context,
                                           TRI_action_t const* action,
                                           v8::Handle<v8::Function> callback,
                                           HttpRequest* request);

// -----------------------------------------------------------------------------
// --SECTION--                                                     private types
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                 class v8_action_t
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8Actions
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief action description for V8
////////////////////////////////////////////////////////////////////////////////

class v8_action_t : public TRI_action_t {
  public:

////////////////////////////////////////////////////////////////////////////////
/// @brief constructor
////////////////////////////////////////////////////////////////////////////////

    v8_action_t ()  {
      _type = "JAVASCRIPT";
    }

////////////////////////////////////////////////////////////////////////////////
/// @brief destructor
////////////////////////////////////////////////////////////////////////////////

    ~v8_action_t () {
      WRITE_LOCKER(_callbacksLock);

      for (map< void*, v8::Persistent<v8::Function> >::iterator i = _callbacks.begin();
           i != _callbacks.end();
           ++i) {
        (i->second).Dispose();
      }
    }

////////////////////////////////////////////////////////////////////////////////
/// @brief creates callback for a context
////////////////////////////////////////////////////////////////////////////////

    void createCallback (void* context, void* callback) {
      WRITE_LOCKER(_callbacksLock);

      map< void*, v8::Persistent<v8::Function> >::iterator i = _callbacks.find(context);

      if (i != _callbacks.end()) {
        (i->second).Dispose();
      }

      v8::Handle<v8::Function>* cb = (v8::Handle<v8::Function>*) callback;
      _callbacks[context] = v8::Persistent<v8::Function>::New(*cb);
    }

////////////////////////////////////////////////////////////////////////////////
/// @brief creates callback for a context
////////////////////////////////////////////////////////////////////////////////

    HttpResponse* execute (TRI_vocbase_t* vocbase, void* context, HttpRequest* request) {
      v8::Handle<v8::Function> callback;

      {
        READ_LOCKER(_callbacksLock);

        map< void*, v8::Persistent<v8::Function> >::iterator i = _callbacks.find(context);

        if (i == _callbacks.end()) {
          LOGGER_WARNING << "no callback function for JavaScript action '" << _url.c_str() << "'";
          return new HttpResponse(HttpResponse::NOT_FOUND);
        }

        callback = i->second;
      }

      return ExecuteActionVocbase(vocbase, context, this, callback, request);
    }

  private:

////////////////////////////////////////////////////////////////////////////////
/// @brief callback dictionary
////////////////////////////////////////////////////////////////////////////////

    map< void*, v8::Persistent<v8::Function> > _callbacks;

////////////////////////////////////////////////////////////////////////////////
/// @brief lock for the callback dictionary
////////////////////////////////////////////////////////////////////////////////

    ReadWriteLock _callbacksLock;
};

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                 private functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8Actions
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief parses the action options "parameters" field of type string
////////////////////////////////////////////////////////////////////////////////

static void ParseActionOptionsParameter (TRI_v8_global_t* v8g,
                                         TRI_action_t* action,
                                         string const& key,
                                         string const& parameter) {
  TRI_action_parameter_type_e p;

  if (parameter == "collection") {
    p = TRI_ACT_COLLECTION;
  }
  if (parameter == "collection-name") {
    p = TRI_ACT_COLLECTION_NAME;
  }
  else if (parameter == "collection-identifier") {
    p = TRI_ACT_COLLECTION_ID;
  }
  else if (parameter == "number") {
    p = TRI_ACT_NUMBER;
  }
  else if (parameter == "string") {
    p = TRI_ACT_STRING;
  }
  else {
    LOG_ERROR("unknown parameter type '%s', falling back to string", parameter.c_str());
    p = TRI_ACT_STRING;
  }

  action->_parameters[key] = p;
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parses the action options "parameters" field
////////////////////////////////////////////////////////////////////////////////

static void ParseActionOptionsParameter (TRI_v8_global_t* v8g,
                                         TRI_action_t* action,
                                         string const& key,
                                         v8::Handle<v8::Value> parameter) {
  if (parameter->IsString() || parameter->IsStringObject()) {
    ParseActionOptionsParameter(v8g, action, key, TRI_ObjectToString(parameter));
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parses the action options "parameters" field
////////////////////////////////////////////////////////////////////////////////

static void ParseActionOptionsParameters (TRI_v8_global_t* v8g,
                                          TRI_action_t* action,
                                          v8::Handle<v8::Object> parameters) {
  v8::Handle<v8::Array> keys = parameters->GetOwnPropertyNames();
  uint32_t len = keys->Length();

  for (uint32_t i = 0;  i < len;  ++i) {
    v8::Handle<v8::Value> key = keys->Get(i);

    ParseActionOptionsParameter(v8g, action, TRI_ObjectToString(key), parameters->Get(key));
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief parses the action options
////////////////////////////////////////////////////////////////////////////////

static void ParseActionOptions (TRI_v8_global_t* v8g,
                                TRI_action_t* action,
                                v8::Handle<v8::Object> options) {

  // check "parameters" field
  if (options->Has(v8g->ParametersKey)) {
    v8::Handle<v8::Value> parameters = options->Get(v8g->ParametersKey);

    if (parameters->IsObject()) {
      ParseActionOptionsParameters(v8g, action, parameters->ToObject());
    }
  }

  // check the "prefix" field
  if (options->Has(v8g->PrefixKey)) {
    action->_isPrefix = TRI_ObjectToBoolean(options->Get(v8g->PrefixKey));
  }
  else {
    action->_isPrefix = false;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @brief executes an action
////////////////////////////////////////////////////////////////////////////////

static HttpResponse* ExecuteActionVocbase (TRI_vocbase_t* vocbase,
                                           void* context,
                                           TRI_action_t const* action,
                                           v8::Handle<v8::Function> callback,
                                           HttpRequest* request) {
  TRI_v8_global_t* v8g;

  v8::HandleScope scope;
  v8::TryCatch tryCatch;

  v8g = (TRI_v8_global_t*) ((v8::Isolate*) context)->GetData();

  // setup the request
  v8::Handle<v8::Object> req = v8::Object::New();

  // Example:
  //      {
  //        "suffix" : [
  //          "suffix1",
  //          "suffix2"
  //        ],
  //
  //        "parameters" : {
  //          "init" : "true"
  //        },
  //
  //        "headers" : {
  //          "accept" : "text/html",
  //          "accept-encoding" : "gzip, deflate",
  //          "accept-language" : "de-de,en-us;q=0.7,en;q=0.3",
  //          "user-agent" : "Mozilla/5.0"
  //        },
  //
  //        "requestType" : "GET",
  //        "requestBody" : "... only for PUT and POST ..."
  //      }

  // copy suffix
  v8::Handle<v8::Array> suffixArray = v8::Array::New();
  vector<string> const& suffix = request->suffix();

  uint32_t index = 0;

  for (size_t s = action->_urlParts;  s < suffix.size();  ++s) {
    suffixArray->Set(index++, v8::String::New(suffix[s].c_str()));
  }

  req->Set(v8g->SuffixKey, suffixArray);

  // copy header fields
  v8::Handle<v8::Object> headerFields = v8::Object::New();

  map<string, string> const& headers = request->headers();
  map<string, string>::const_iterator iter = headers.begin();

  for (; iter != headers.end(); ++iter) {
    headerFields->Set(v8::String::New(iter->first.c_str()), v8::String::New(iter->second.c_str()));
  }

  req->Set(v8g->HeadersKey, headerFields);

  // copy request type
  switch (request->requestType()) {
    case HttpRequest::HTTP_REQUEST_POST:
      req->Set(v8g->RequestTypeKey, v8g->PostConstant);
      req->Set(v8g->RequestBodyKey, v8::String::New(request->body().c_str()));
      break;

    case HttpRequest::HTTP_REQUEST_PUT:
      req->Set(v8g->RequestTypeKey, v8g->PutConstant);
      req->Set(v8g->RequestBodyKey, v8::String::New(request->body().c_str()));
      break;

    case HttpRequest::HTTP_REQUEST_DELETE:
      req->Set(v8g->RequestTypeKey, v8g->DeleteConstant);
      break;

    case HttpRequest::HTTP_REQUEST_HEAD:
      req->Set(v8g->RequestTypeKey, v8g->HeadConstant);
      break;

    default:
      req->Set(v8g->RequestTypeKey, v8g->GetConstant);
      break;
  }

  // copy request parameter
  v8::Handle<v8::Array> parametersArray = v8::Array::New();
  map<string, string> values = request->values();

  for (map<string, string>::iterator i = values.begin();  i != values.end();  ++i) {
    string const& k = i->first;
    string const& v = i->second;

    map<string, TRI_action_parameter_type_e>::const_iterator p = action->_parameters.find(k);

    if (p == action->_parameters.end()) {
      parametersArray->Set(v8::String::New(k.c_str()), v8::String::New(v.c_str()));
    }
    else {
      TRI_action_parameter_type_e const& ap = p->second;

      switch (ap) {
        case TRI_ACT_COLLECTION: {
          if (! v.empty()) {
            char ch = v[0];
            TRI_vocbase_col_t const* collection = 0;

            if ('0' < ch && ch <= '9') {
              collection = TRI_LookupCollectionByIdVocBase(vocbase, TRI_UInt64String(v.c_str()));
            }
            else {
              collection = TRI_FindCollectionByNameVocBase(vocbase, v.c_str(), false);
            }

            if (collection != 0) {
              parametersArray->Set(v8::String::New(k.c_str()), TRI_WrapCollection(collection));
            }
          }

          break;
        }

        case TRI_ACT_COLLECTION_NAME: {
          TRI_vocbase_col_t const* collection = TRI_FindCollectionByNameVocBase(vocbase, v.c_str(), false);

          if (collection != 0) {
            parametersArray->Set(v8::String::New(k.c_str()), TRI_WrapCollection(collection));
          }

          break;
        }

        case TRI_ACT_COLLECTION_ID: {
          TRI_vocbase_col_t const* collection = TRI_LookupCollectionByIdVocBase(
            vocbase,
            TRI_UInt64String(v.c_str()));

          if (collection != 0) {
            parametersArray->Set(v8::String::New(k.c_str()), TRI_WrapCollection(collection));
          }

          break;
        }

        case TRI_ACT_NUMBER:
          parametersArray->Set(v8::String::New(k.c_str()), v8::Number::New(TRI_DoubleString(v.c_str())));
          break;

        case TRI_ACT_STRING: {
          parametersArray->Set(v8::String::New(k.c_str()), v8::String::New(v.c_str()));
          break;
        }
      }
    }
  }

  req->Set(v8g->ParametersKey, parametersArray);

  // execute the callback
  v8::Handle<v8::Object> res = v8::Object::New();
  v8::Handle<v8::Value> args[2] = { req, res };

  callback->Call(callback, 2, args);

  // convert the result
  if (tryCatch.HasCaught()) {
    string msg = TRI_StringifyV8Exception(&tryCatch);

    HttpResponse* response = new HttpResponse(HttpResponse::SERVER_ERROR);
    response->body().appendText(msg);
    return response;
  }

  else {
    HttpResponse::HttpResponseCode code = HttpResponse::OK;

    if (res->Has(v8g->ResponseCodeKey)) {
      code = (HttpResponse::HttpResponseCode) TRI_ObjectToDouble(res->Get(v8g->ResponseCodeKey));
    }

    HttpResponse* response = new HttpResponse(code);

    if (res->Has(v8g->ContentTypeKey)) {
      response->setContentType(TRI_ObjectToString(res->Get(v8g->ContentTypeKey)));
    }

    if (res->Has(v8g->BodyKey)) {
      response->body().appendText(TRI_ObjectToString(res->Get(v8g->BodyKey)));
    }

    if (res->Has(v8g->HeadersKey)) {
      v8::Handle<v8::Value> val = res->Get(v8g->HeadersKey);
      v8::Handle<v8::Object> v8Headers = val.As<v8::Object>();

      if (v8Headers->IsObject()) {
        v8::Handle<v8::Array> props = v8Headers->GetPropertyNames();

        for (uint32_t i = 0; i < props->Length(); i++) {
          v8::Handle<v8::Value> key = props->Get(v8::Integer::New(i));
          response->setHeader(TRI_ObjectToString(key), TRI_ObjectToString(v8Headers->Get(key)));
        }
      }
    }

    return response;
  }
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                      JS functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8Actions
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief defines a new action
///
/// @FUN{defineAction(@FA{name}, @FA{callback}, @FA{parameter})}
////////////////////////////////////////////////////////////////////////////////

static v8::Handle<v8::Value> JS_DefineAction (v8::Arguments const& argv) {
  v8::Isolate* isolate;

  TRI_v8_global_t* v8g;
  v8::HandleScope scope;

  isolate = v8::Isolate::GetCurrent();
  v8g = (TRI_v8_global_t*) isolate->GetData();

  if (argv.Length() != 3) {
    return scope.Close(v8::ThrowException(v8::String::New("usage: SYS_DEFINE_ACTION(<name>, <callback>, <parameter>)")));
  }

  // extract the action name
  v8::String::Utf8Value utf8name(argv[0]);

  if (*utf8name == 0) {
    return scope.Close(v8::ThrowException(v8::String::New("<name> must be an UTF8 name")));
  }

  string name = *utf8name;

  // extract the action callback
  if (! argv[1]->IsFunction()) {
    return scope.Close(v8::ThrowException(v8::String::New("<callback> must be a function")));
  }

  v8::Handle<v8::Function> callback = v8::Handle<v8::Function>::Cast(argv[1]);

  // extract the options
  v8::Handle<v8::Object> options;

  if (argv[2]->IsObject()) {
    options = argv[2]->ToObject();
  }
  else {
    options = v8::Object::New();
  }

  // create an action with the given options
  v8_action_t* action = new v8_action_t;
  ParseActionOptions(v8g, action, options);

  // store an action with the given name
  TRI_action_t* result = TRI_DefineActionVocBase(name, action);

  // and define the callback
  if (result != 0) {
    result->createCallback((void*) isolate, (void*) &callback);
  }

  return scope.Close(v8::Undefined());
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup V8Actions
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief stores the V8 actions function inside the global variable
////////////////////////////////////////////////////////////////////////////////

void TRI_InitV8Actions (v8::Handle<v8::Context> context,
                        string const& actionQueue,
                        set<string> const& allowedContexts) {
  v8::HandleScope scope;

  // check the isolate
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  TRI_v8_global_t* v8g = (TRI_v8_global_t*) isolate->GetData();

  if (v8g == 0) {
    v8g = new TRI_v8_global_t;
    isolate->SetData(v8g);
  }

  // .............................................................................
  // create the global constants
  // .............................................................................

  context->Global()->Set(v8::String::New("SYS_ACTION_QUEUE"),
                         v8::String::New(actionQueue.c_str()),
                         v8::ReadOnly);

  v8::Handle<v8::Array> contexts = v8::Array::New();
  size_t pos = 0;

  for (set<string>::iterator i = allowedContexts.begin();  i != allowedContexts.end();  ++i, ++pos) {
    contexts->Set(pos, v8::String::New((*i).c_str()));
  }

  context->Global()->Set(v8::String::New("SYS_ACTION_CONTEXTS"),
                         contexts,
                         v8::ReadOnly);

  // .............................................................................
  // create the global functions
  // .............................................................................

  context->Global()->Set(v8::String::New("SYS_DEFINE_ACTION"),
                         v8::FunctionTemplate::New(JS_DefineAction)->GetFunction(),
                         v8::ReadOnly);

  // .............................................................................
  // keys
  // .............................................................................

  v8g->BodyKey = v8::Persistent<v8::String>::New(v8::String::New("body"));
  v8g->ContentTypeKey = v8::Persistent<v8::String>::New(v8::String::New("contentType"));
  v8g->HeadersKey = v8::Persistent<v8::String>::New(v8::String::New("headers"));
  v8g->ParametersKey = v8::Persistent<v8::String>::New(v8::String::New("parameters"));
  v8g->PrefixKey = v8::Persistent<v8::String>::New(v8::String::New("prefix"));
  v8g->RequestBodyKey = v8::Persistent<v8::String>::New(v8::String::New("requestBody"));
  v8g->RequestTypeKey = v8::Persistent<v8::String>::New(v8::String::New("requestType"));
  v8g->ResponseCodeKey = v8::Persistent<v8::String>::New(v8::String::New("responseCode"));
  v8g->SuffixKey = v8::Persistent<v8::String>::New(v8::String::New("suffix"));

  v8g->DeleteConstant = v8::Persistent<v8::String>::New(v8::String::New("DELETE"));
  v8g->GetConstant = v8::Persistent<v8::String>::New(v8::String::New("GET"));
  v8g->HeadConstant = v8::Persistent<v8::String>::New(v8::String::New("HEAD"));
  v8g->PostConstant = v8::Persistent<v8::String>::New(v8::String::New("POST"));
  v8g->PutConstant = v8::Persistent<v8::String>::New(v8::String::New("PUT"));
}

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// {@inheritDoc}\\|/// @addtogroup\\|/// @page\\|// --SECTION--\\|/// @\\}\\)"
// End: