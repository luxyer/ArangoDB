////////////////////////////////////////////////////////////////////////////////
/// @brief hash functions
///
/// @file
///
/// DISCLAIMER
///
/// Copyright 2010-2011 triagens GmbH, Cologne, Germany
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
/// @author Copyright 2011-2010, triAGENS GmbH, Cologne, Germany
////////////////////////////////////////////////////////////////////////////////

#ifndef TRIAGENS_PHILADELPHIA_BASICS_HASHES_H
#define TRIAGENS_PHILADELPHIA_BASICS_HASHES_H 1

#include <Basics/Common.h>

#ifdef __cplusplus
extern "C" {
#endif

// -----------------------------------------------------------------------------
// --SECTION--                                                               FNV
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Hashes Hashes
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief computes a NVL hash for blobs
////////////////////////////////////////////////////////////////////////////////

uint64_t TRI_FnvHashBlob (TRI_blob_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief computes a NVL hash for memory blobs
////////////////////////////////////////////////////////////////////////////////

uint64_t TRI_FnvHashPointer (void const*, size_t);

////////////////////////////////////////////////////////////////////////////////
/// @brief computes a NVL hash for strings
////////////////////////////////////////////////////////////////////////////////

uint64_t TRI_FnvHashString (char const*);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                             CRC32
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Hashes Hashes
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief initial CRC32 value
////////////////////////////////////////////////////////////////////////////////

uint32_t TRI_InitialCrc32 (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief final CRC32 value
////////////////////////////////////////////////////////////////////////////////

uint32_t TRI_FinalCrc32 (uint32_t);

////////////////////////////////////////////////////////////////////////////////
/// @brief CRC32 value of data block
////////////////////////////////////////////////////////////////////////////////

uint32_t TRI_BlockCrc32 (uint32_t, char const* data, size_t length);

////////////////////////////////////////////////////////////////////////////////
/// @brief computes a CRC32 for blobs
////////////////////////////////////////////////////////////////////////////////

uint32_t TRI_Crc32HashBlob (TRI_blob_t*);

////////////////////////////////////////////////////////////////////////////////
/// @brief computes a CRC32 for memory blobs
////////////////////////////////////////////////////////////////////////////////

uint32_t TRI_Crc32HashPointer (void const*, size_t);

////////////////////////////////////////////////////////////////////////////////
/// @brief computes a CRC32 for strings
////////////////////////////////////////////////////////////////////////////////

uint64_t TRI_Crc32HashString (char const*);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

// -----------------------------------------------------------------------------
// --SECTION--                                                            MODULE
// -----------------------------------------------------------------------------

// -----------------------------------------------------------------------------
// --SECTION--                                                  public functions
// -----------------------------------------------------------------------------

////////////////////////////////////////////////////////////////////////////////
/// @addtogroup Hashes Hashes
/// @{
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
/// @brief initialises the hashes components
////////////////////////////////////////////////////////////////////////////////

void TRI_InitialiseHashes (void);

////////////////////////////////////////////////////////////////////////////////
/// @brief shut downs the hashes components
////////////////////////////////////////////////////////////////////////////////

void TRI_ShutdownHashes (void);

////////////////////////////////////////////////////////////////////////////////
/// @}
////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif

#endif

// Local Variables:
// mode: outline-minor
// outline-regexp: "^\\(/// @brief\\|/// @addtogroup\\|// --SECTION--\\)"
// End:
