/* Copyright (c) 2017, Computer History Museum 

   All rights reserved. 

   Redistribution and use in source and binary forms, with or without 
   modification, are permitted (subject to the limitations in the disclaimer
   below) provided that the following conditions are met: 

   * Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer. 
   * Redistributions in binary form must reproduce the above copyright notice, 
     this list of conditions and the following disclaimer in the documentation
     and/or other materials provided with the distribution. 
   * Neither the name of Computer History Museum nor the names of its
     contributors may be used to endorse or promote products derived from this
     software without specific prior written permission. 

   NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
   THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
   CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
   NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
   PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
   CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
   EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
   PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
   OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
   WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
   OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
   ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SPELL_H
#define SPELL_H

Boolean HaveSpeller(void);

#ifdef WINTERTREE
short SpellOpen(void);
void SpellScan(void);
void PeteSpellScan(PETEHandle pte, Boolean manual);
void SpellClose(Boolean withPrejuidice);
void EnableSpellMenu(Boolean all);
void SpellMenu(short item, short modifiers);
void SpellAgain(void);
Boolean SpellAnyWrongHuh(PETEHandle pte);
#ifdef USECMM
OSErr AppendSpellItems(PETEHandle pte, MenuHandle contextMenu);
#endif
typedef enum { sprNeverSpell = -2, sprSpellComplete = -1 } SpellResultEnum;
#define SpelledAuto(pte) ((*PeteExtra(pte))->spelled==sprSpellComplete)
#define SpellDisabled(pte) ((*PeteExtra(pte))->spelled == sprNeverSpell)
#define SpellChecked(pte) (!SpellDisabled(pte) && (*PeteExtra(pte))->spelled<0)

#define SpellOptOnQueue	(0!=(WinterTreeOptions&(1L<<22)))
#define SpellOptGuesses (0==(WinterTreeOptions&(1L<<21)))

#endif
#endif
