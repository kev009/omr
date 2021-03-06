/*******************************************************************************
 * Copyright (c) 2000, 2018 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "x/codegen/DataSnippet.hpp"

#include <stdint.h>                   // for uint8_t, int16_t, uint32_t, etc
#include <string.h>                   // for NULL, memcpy
#include "codegen/CodeGenerator.hpp"  // for CodeGenerator
#include "compile/Compilation.hpp"    // for Compilation
#include "env/CompilerEnv.hpp"
#include "env/IO.hpp"
#include "env/TRMemory.hpp"           // for TR_HeapMemory
#include "env/jittypes.h"             // for intptrj_t
#include "il/symbol/LabelSymbol.hpp"  // for LabelSymbol
#include "ras/Debug.hpp"              // for TR_Debug

namespace TR { class Node; }

TR::X86DataSnippet::X86DataSnippet(TR::CodeGenerator *cg, TR::Node * n, void *c, uint8_t size)
   : TR::Snippet(cg, n, TR::LabelSymbol::create(cg->trHeapMemory(),cg), false),
     _data(size, 0, getTypedAllocator<uint8_t>(TR::comp()->allocator()))
   {
   if (c)
      memcpy(_data.data(), c, size);
   _isClassAddress = false;
   }


void
TR::X86DataSnippet::addMetaDataForCodeAddress(uint8_t *cursor)
   {
   // add dummy class unload/redefinition assumption.
   if (_isClassAddress)
      {
      if (TR::Compiler->target.is64Bit())
         {
         cg()->jitAddPicToPatchOnClassUnload((void*)-1, (void *) cursor);
         if (cg()->wantToPatchClassPointer(NULL, cursor)) // unresolved
            {
            cg()->jitAddPicToPatchOnClassRedefinition(((void *) -1), (void *) cursor, true);
            }
         }
      else
         {
         cg()->jitAdd32BitPicToPatchOnClassUnload((void*)-1, (void *) cursor);
         if (cg()->wantToPatchClassPointer(NULL, cursor)) // unresolved
            {
            cg()->jitAdd32BitPicToPatchOnClassRedefinition(((void *) -1), (void *) cursor, true);
            }
         }
      }
   }


uint8_t *TR::X86DataSnippet::emitSnippetBody()
   {
   uint8_t *cursor = cg()->getBinaryBufferCursor();

   // align to 16 bytes
   if (getDataSize() % 16 == 0)
      {
      cursor = (uint8_t*)(((intptrj_t)(cursor + 15)) & ((-1)<<4));
      }

   getSnippetLabel()->setCodeLocation(cursor);

   memcpy(cursor, getRawData(), getDataSize());

   addMetaDataForCodeAddress(cursor);


   cursor += getDataSize();

   return cursor;
   }

void TR::X86DataSnippet::printValue(TR::FILE* pOutFile, TR_Debug* debug)
   {
   if (pOutFile == NULL)
      return;

   switch (getDataSize())
      {
      case 2:
         trfprintf(pOutFile, "0x%04x | %d",
                   0xffff & (int32_t)getData<int16_t>(),
                   (int32_t)getData<int16_t>());
         break;
      case 4:
         trfprintf(pOutFile, "0x%08x | %d | float %g",
                   getData<int32_t>(),
                   getData<int32_t>(),
                   getData<float>());
         break;
      case 8:
         trfprintf(pOutFile, "0x%016llx | %lld | double %g",
                   getData<int64_t>(),
                   getData<int64_t>(),
                   getData<double>());
         break;
      default:
         trfprintf(pOutFile, "VECTOR VALUE");
         break;
      }
   }

void TR::X86DataSnippet::print(TR::FILE* pOutFile, TR_Debug* debug)
   {
   if (pOutFile == NULL)
      return;

   uint8_t *bufferPos = getSnippetLabel()->getCodeLocation();

   debug->printSnippetLabel(pOutFile, getSnippetLabel(), bufferPos, debug->getName(this));
   debug->printPrefix(pOutFile, NULL, bufferPos, getDataSize());

   const char* toString;
   switch (getDataSize())
      {
      case 8:
         toString = dqString();
         break;
      case 4:
         toString = ddString();
         break;
      case 2:
         toString = dwString();
         break;
      default:
         toString = dbString();
         break;
      }
   trfprintf(pOutFile, "%s \t%s", toString, hexPrefixString());

   for (int i=getDataSize()-1; i >= 0; i--)
      {
      trfprintf(pOutFile, "%02x", bufferPos[i]);
      }

   trfprintf(pOutFile,"%s\t%s ", hexSuffixString(), commentString());
   printValue(pOutFile, debug);
   }
