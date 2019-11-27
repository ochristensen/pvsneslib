;---------------------------------------------------------------------------------
;
;	Copyright (C) 2015
;		Alekmaul 
;
;	This software is provided 'as-is', without any express or implied
;	warranty.  In no event will the authors be held liable for any
;	damages arising from the use of this software.
;
;	Permission is granted to anyone to use this software for any
;	purpose, including commercial applications, and to alter it and
;	redistribute it freely, subject to the following restrictions:
;
;	1.	The origin of this software must not be misrepresented; you
;		must not claim that you wrote the original software. If you use
;		this software in a product, an acknowledgment in the product
;		documentation would be appreciated but is not required.
;	2.	Altered source versions must be plainly marked as such, and
;		must not be misrepresented as being the original software.
;	3.	This notice may not be removed or altered from any source
;		distribution.
;
;---------------------------------------------------------------------------------

.ramsection ".reg_scores" bank 0 slot 1
scorestring		dsb 10                         ; for score to string convertion
.ends

.section ".scores_text" superfree

;---------------------------------------------------------------------------------
; void scoreClear(scoMemory *source);
scoreClear:
	php
	phb
	phx

	sep	#$20
	lda	10,s 							; bank address of score
	pha
	plb
	
	rep	#$20							; word address of score
	lda	8,s
	tax
	
	lda #$0000.w						; clear score
	sta 0,x
	sta 2,x
	
	plx
	
	plb
	plp
	rtl

;---------------------------------------------------------------------------------
; void scoreAdd(scoMemory *source, u16 value);
scoreAdd:
	php
	phb
    
	phx
		
	sep	#$20
	lda	10,s 							; bank address of score
	pha
	plb

	rep	#$20							; word address of score
	lda	8,s
	tax
	
	lda 12,s							; get value and add it
	clc
	adc.w 0,x
	sta 0,x
	sec
	sbc #$2710						; is it greater than 10000 ?
	bcc _scoAdd1					; not a overflow, exit 
	
	sta 0,x								; store again when 10000 is substract
	lda 2,x								; add one to high number
	inc a
	sta 2,x
	
_scoAdd1:
	
	plx
	
	plb
	plp
	rtl
	
;---------------------------------------------------------------------------------
; void scoreCpy(scoMemory *source, scoMemory *dest);
scoreCpy:
	php
	phb
    
	phx

	sep	#$20
	lda	10,s 							; bank address of source score
	pha
	plb

	rep	#$20							; word address of source score
	lda	8,s
	tax

	lda 0,x								; push score on stack (word = 0, bank = 2)
	pha
	lda 2,x
	pha
	
	sep	#$20
	lda	18,s 							; bank address of dest score (14+4 because of pha)
	pha
	plb

	rep	#$20							; word address of dest score (12+4 because of pha)
	lda	16,s
	tax

	pla										; put source score
	sta 2,x
	pla
	sta 0,x

	plx
	
	plb
	plp
	rtl

;---------------------------------------------------------------------------------
; void scoreCmp(scoMemory *source, scoMemory *dest);
scoreCmp:
	php
	phb
    
	phx
	phy
	
	sep	#$20
	lda	12,s 							; bank address of source score
	pha
	plb

	rep	#$20							; word address of source score
	lda	10,s
	tax
	
	lda 0,x								; push score on stack
	pha
	lda 2,x
	pha
	
	sep	#$20
	lda	20,s 							; bank address of dest score
	pha
	plb

	rep	#$20							; word address of dest score
	lda	18,s
	tay
	
	pla
	sec
	sbc 2,y								; is high equals ?
	beq _scocmpequ				; high = low; must check lower word
	bcs _scocmphig				; nope, it is lower
	bra _scocmplow

_scocmpequ:
	pla
	sec
	sbc 0,y								; is low equals ?
	beq _scocmpequ1				; ok, retrun 0
	bcs _scocmphig1				; nope, it is lower
	bra _scocmplow1
	
_scocmpequ1:
	sep #$20
  lda.b #0
  sta.b tcc__r0
	bra _scocmpbye

_scocmphig:
	pla
_scocmphig1:
	sep #$20
  lda.b #-1
  sta.b tcc__r0
	bra _scocmpbye
	
_scocmplow:
	pla
_scocmplow1:
	sep #$20
  lda.b #1
  sta.b tcc__r0

_scocmpbye:
	ply
	plx
	
	plb
	plp
	rtl

;---------------------------------------------------------------------------------
; void score2Str(scoMemory *source, u16 value);
score2Str:	
	php
	phb
    
	phx
	
	plx
	
	plb
	plp
	rtl
	
    ;            pop de
    ;            pop hl
    ;            pop bc
    ;            push bc
    ;            push hl
    ;            push de
    ;            
  ;              push bc
;
;                ld e,(hl)
;                inc hl;
              ; 
 ;ld d,(hl)
   ;             inc hl
                
    ;            push hl
                
     ;           ld bc,#score_string+4
                
      ;          push bc
       ;         push de
        ;        call _utoa0
         ;       pop bc
          ;      pop bc
                
 ;               pop hl

 ;               ld e,(hl)
 ;               inc hl
 ;               ld d,(hl)
                
 ;               ld bc,#score_string
                
  ;              push bc
  ;              push de
  ;              call _utoa0
  ;              pop bc
  ;              pop bc
                
  ;              xor a
  ;              ld (score_string+9),a
                
  ;              pop bc
  ;              ld hl,#9
   ;             sbc hl,bc
   ;             ld de,#score_string
   ;             add hl,de
                
    ;            ret
				
				
;_score_cmp_gt:
;                pop bc
;                pop de
;                pop hl
;                push hl
;                push de
;;                push bc
;                
;                push de
;                push hl
;                
;                call _score_cmp_lt
;                
;                pop bc
;                pop bc
;                
;                ret
								
;_score_cmp_lt:
;                pop bc
;                pop de
;                pop hl
;                push hl
;                push de
;                push bc
;                
;                push hl
;                push de
;                
;                inc hl
;                inc hl
;                ld c,(hl)
;                inc hl
;                ld b,(hl)

;                ex de,hl

;                inc hl
;                inc hl
;                ld a,(hl)
;                inc hl
;                ld h,(hl)
;                ld l,a
;                
;                or a
;                sbc hl,bc
;                
;                jr c,$1
;                jr nz,$0
;                
;                pop de
;                pop hl
;                push hl
;                push de
;                
;                ld c,(hl)
;                inc hl
;                ld b,(hl)

;                ex de,hl

;                ld a,(hl)
;                inc hl
;                ld h,(hl)
;                ld l,a
;                
;                sbc hl,bc
;                
;                jr c,$1;

;$0:
;                ld hl,#0x0000
;                jr $2
;$1:
;                ld hl,#0x0001
;$2:
;                pop bc
;                pop bc
;                ret

.ends

