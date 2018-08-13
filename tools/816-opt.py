#!/usr/bin/env python3

import argparse
import os
import re
import sys


ST_TO_PSEUDOREG = re.compile('st([axyz]).b tcc__([rf][0-9]*h?)$')
ST_XY_TO_PSEUDOREG = re.compile('st([xy]).b tcc__([rf][0-9]*h?)$')
ST_A_TO_PSEDUOREG = re.compile('sta.b tcc__([rf][0-9]*h?)$')

def is_control(line):
    """check if the line alters control flow"""
    return len(line) > 0 and line[0] in 'jb+-' or line.endswith(':')


def changes_accu(line):
    """check if the line alters the accumulator"""
    return (line[2] == 'a' and not line[:3] in ['pha', 'sta']) or (len(line) == 5 and line.endswith(' a'))


def skip_redundant_st(i, text, r):
    for j in range(i+1, min(len(text), i+30)):
        store_match = re.match("st([axyz]).b tcc__{0}$".format(r), text[j])
        if store_match:
            return True  # another store to the same pregister
        if text[j].startswith('jsr.l ') and not text[j].startswith('jsr.l tcc__'):
            # before function call (will be clobbered anyway)
            return True
        # cases in which we don't pursue optimization further
        if is_control(text[j]) or ('tcc__' + r) in text[j]:
            # branch or other use of the preg
            return False
        if r.endswith('h') and ('[tcc__' + r.rstrip('h')) in text[j]:
            # use as a pointer
            return False
    return False


def main(in_file, out_file, verbose):
    # optimize only uncommented lines
    text_raw = open(in_file, 'r').readlines()
    text = [l.strip() for l in text_raw if not l.startswith(";")]

    # find .bss section symbols
    bss = []
    in_bss = False
    for l in text:
        if l == '.ramsection ".bss" bank $7e slot 2':
            in_bss = True
            continue
        if l == '.ends':
            in_bss = False
        if in_bss:
            bss += [l.split(' ')[0]]
    if in_bss:
        print("unterminated bss section", file=sys.stderr)
        sys.exit(1)
    if verbose:
        print("bss:\n{0}".format(bss))

    opts_total = 0  # total number of optimizations performed
    opts_this_pass = 1  # have we optimized in this pass?
    opt_pass_ct = 0  # optimization pass counter
    while opts_this_pass:
        opt_pass_ct += 1
        if verbose:
            print("pass {0}:".format(opt_pass_ct))
        opts_this_pass = 0
        text_opt = []  # optimized code array, will be filled in during this pass
        i = 0
        while i < len(text):
            if text[i].startswith('st'):
                # stores (accu/x/y/zero) to pseudo-registers
                r = ST_TO_PSEUDOREG.match(text[i])
                # eliminate redundant stores
                if r and skip_redundant_st(i, text, r.groups()[1]):
                    i += 1  # skip redundant store
                    opts_this_pass += 1
                    continue

                # stores (x/y) to pseudo-registers
                r = ST_XY_TO_PSEUDOREG.match(text[i])
                if r:
                    # store hwreg to preg, push preg, function call -> push hwreg, function call
                    if text[i+1] == 'pei (tcc__' + r.groups()[1] + ')' and text[i+2].startswith('jsr.l '):
                        text_opt += ['ph' + r.groups()[0]]
                        i += 2
                        opts_this_pass += 1
                        continue
                    # store hwreg to preg, push preg -> store hwreg to preg, push hwreg (shorter)
                    if text[i+1] == 'pei (tcc__' + r.groups()[1] + ')':
                        text_opt += [text[i]]
                        text_opt += ['ph' + r.groups()[0]]
                        i += 2
                        opts_this_pass += 1
                        continue
                    # store hwreg to preg, load hwreg from preg -> store hwreg to preg, transfer hwreg/hwreg (shorter)
                    if text[i+1] == 'lda.b tcc__' + r.groups()[1] or text[i+1] == 'lda.b tcc__' + r.groups()[1] + " ; DON'T OPTIMIZE":
                        text_opt += [text[i]]
                        # FIXME: shouldn't this be marked as DON'T OPTIMIZE again?
                        text_opt += ['t' + r.groups()[0] + 'a']
                        i += 2
                        opts_this_pass += 1
                        continue

                # stores (accu only) to pseudo-registers
                r = ST_A_TO_PSEDUOREG.match(text[i])
                if r:
                    # store preg followed by load preg
                    if text[i+1] == 'lda.b tcc__' + r.groups()[0]:
                        text_opt += [text[i]]  # keep store
                        i += 2  # omit load
                        opts_this_pass += 1
                        continue
                    # store preg followed by load preg with ldx/ldy in between
                    if (text[i+1].startswith('ldx') or text[i+1].startswith('ldy')) and text[i+2] == 'lda.b tcc__' + r.groups()[0]:
                        text_opt += [text[i]]  # keep store
                        text_opt += [text[i+1]]
                        i += 3  # omit load
                        opts_this_pass += 1
                        continue
                    # store accu to preg, push preg, function call -> push accu, function call
                    if text[i+1] == 'pei (tcc__' + r.groups()[0] + ')' and text[i+2].startswith('jsr.l '):
                        text_opt += ['pha']
                        i += 2
                        opts_this_pass += 1
                        continue
                    # store accu to preg, push preg -> store accu to preg, push accu (shorter)
                    if text[i+1] == 'pei (tcc__' + r.groups()[0] + ')':
                        text_opt += [text[i]]
                        text_opt += ['pha']
                        i += 2
                        opts_this_pass += 1
                        continue
                    # store accu to preg1, push preg2, push preg1 -> store accu to preg1, push preg2, push accu
                    elif text[i+1].startswith('pei ') and text[i+2] == 'pei (tcc__' + r.groups()[0] + ')':
                        text_opt += [text[i+1]]
                        text_opt += [text[i]]
                        text_opt += ['pha']
                        i += 3
                        opts_this_pass += 1
                        continue

                    # convert incs/decs on pregs incs/decs on hwregs
                    cont = False
                    for crem in 'inc', 'dec':
                        if text[i+1] == crem + '.b tcc__' + r.groups()[0]:
                            # store to preg followed by crement on preg
                            if text[i+2] == crem + '.b tcc__' + r.groups()[0] and text[i+3].startswith('lda'):
                                # store to preg followed by two crements on preg
                                # increment the accu first, then store it to preg
                                text_opt += [crem + ' a', crem + ' a',
                                             'sta.b tcc__' + r.groups()[0]]
                                # a subsequent load can be omitted (the right value is already in the accu)
                                if text[i+3] == 'lda.b tcc__' + r.groups()[0]:
                                    i += 4
                                else:
                                    i += 3
                                opts_this_pass += 1
                                cont = True
                                break
                            # text[i+2] == 'lda.b tcc__' + r.groups()[0]:
                            elif text[i+2].startswith('lda'):
                                # same thing with only one crement (FIXME: there should be a more clever way to do this...)
                                text_opt += [crem + ' a',
                                             'sta.b tcc__' + r.groups()[0]]
                                if text[i+2] == 'lda.b tcc__' + r.groups()[0]:
                                    i += 3
                                else:
                                    i += 2
                                opts_this_pass += 1
                                cont = True
                                break
                    if cont:
                        continue

                    r1 = re.match('lda.b tcc__([rf][0-9]*)', text[i+1])
                    if r1:
                        if text[i+2][: 3] in ['and', 'ora']:
                            # store to preg1, load from preg2, and/or preg1 -> store to preg1, and/or preg2
                            if text[i+2][3:] == '.b tcc__' + r.groups()[0]:
                                text_opt += [text[i]]  # store
                                text_opt += [text[i+2][:3] +
                                             '.b tcc__' + r1.groups()[0]]
                                i += 3
                                opts_this_pass += 1
                                continue

                    # store to preg, switch to 8 bits, load from preg => skip the load
                    if text[i+1] == 'sep #$20' and text[i+2] == 'lda.b tcc__' + r.groups()[0]:
                        text_opt += [text[i]]
                        text_opt += [text[i+1]]
                        i += 3  # skip load
                        opts_this_pass += 1
                        continue

                    # two stores to preg without control flow or other uses of preg => skip first store
                    if not is_control(text[i+1]) and not ('tcc__' + r.groups()[0]) in text[i+1]:
                        if text[i+2] == text[i]:
                            text_opt += [text[i+1]]
                            text_opt += [text[i+2]]
                            i += 3  # skip first store
                            opts_this_pass += 1
                            continue

                    # store hwreg to preg, load hwreg from preg -> store hwreg to preg, transfer hwreg/hwreg (shorter)
                    r1 = re.match('ld([xy]).b tcc__' +
                                  r.groups()[0], text[i+1])
                    if r1:
                        text_opt += [text[i]]
                        text_opt += ['ta' + r1.groups()[0]]
                        i += 2
                        opts_this_pass += 1
                        continue

                    # store accu to preg then load accu from preg, with something in-between that does not alter
                    # control flow or touch accu or preg => skip load
                    if not (is_control(text[i+1]) or changes_accu(text[i+1]) or 'tcc__' + r.groups()[0] in text[i+1]):
                        if text[i+2] == 'lda.b tcc__' + r.groups()[0]:
                            text_opt += [text[i]]
                            text_opt += [text[i+1]]
                            i += 3  # skip load
                            opts_this_pass += 1
                            continue

                    # store preg1, clc, load preg2, add preg1 -> store preg1, clc, add preg2
                    if text[i+1] == 'clc':
                        r1 = re.match('lda.b tcc__(r[0-9]*)', text[i+2])
                        if r1 and text[i+3] == 'adc.b tcc__' + r.groups()[0]:
                            text_opt += [text[i]]
                            text_opt += [text[i+1]]
                            text_opt += ['adc.b tcc__' + r1.groups()[0]]
                            i += 4  # skip load
                            opts_this_pass += 1
                            continue

                    # store accu to preg, asl preg => asl accu, store accu to preg
                    # FIXME: is this safe? can we rely on code not making assumptions about the contents of the accu
                    # after the shift?
                    if text[i+1] == 'asl.b tcc__' + r.groups()[0]:
                        text_opt += ['asl a']
                        text_opt += [text[i]]
                        i += 2
                        opts_this_pass += 1
                        continue

                r = re.match('sta (.*),s$', text[i])
                if r:
                    if text[i+1] == 'lda ' + r.groups()[0] + ',s':
                        text_opt += [text[i]]
                        i += 2  # omit load
                        opts_this_pass += 1
                        continue

            elif text[i].startswith('ld'):
                r = re.match('ldx #0', text[i])
                if r:
                    r1 = re.match('lda.l (.*),x$', text[i+1])
                    if r1 and not text[i+3].endswith(',x'):
                        text_opt += ['lda.l ' + r1.groups()[0]]
                        i += 2
                        opts_this_pass += 1
                        continue
                    elif r1:
                        text_opt += ['lda.l ' + r1.groups()[0]]
                        text_opt += [text[i+2]]
                        text_opt += [text[i+3].replace(',x', '')]
                        i += 4
                        opts_this_pass += 1
                        continue

                if text[i] == 'lda.w #0':
                    if text[i+1].startswith('sta.b ') and text[i+2].startswith('lda'):
                        text_opt += [text[i+1].replace('sta.', 'stz.')]
                        i += 2
                        opts_this_pass += 1
                        continue
                elif text[i].startswith('lda.w #'):
                    if text[i+1] == 'sep #$20' and text[i+2].startswith('sta ') and text[i+3] == 'rep #$20' and text[i+4].startswith('lda'):
                        text_opt += ['sep #$20',
                                     text[i].replace('lda.w', 'lda.b'), text[i+2], text[i+3]]
                        i += 4
                        opts_this_pass += 1
                        continue

                if text[i].startswith('lda.b') and not is_control(text[i+1]) and not 'a' in text[i+1] and text[i+2].startswith('lda.b'):
                    text_opt += [text[i+1], text[i+2]]
                    i += 3
                    opts_this_pass += 1
                    continue

                # don't write preg high back to stack if it hasn't been updated
                if text[i+1].endswith('h') and text[i+1].startswith('sta.b tcc__r') and text[i].startswith('lda ') and text[i].endswith(',s'):
                    local = text[i][4:]
                    reg = text[i+1][6:]
                    # lda stack ; store high preg ; ... ; load high preg ; sta stack
                    j = i + 2
                    while j < len(text) - 2 and not is_control(text[j]) and not reg in text[j]:
                        j += 1
                    if text[j] == 'lda.b ' + reg and text[j+1] == 'sta ' + local:
                        while i < j:
                            text_opt += [text[i]]
                            i += 1
                        i += 2  # skip load high preg ; sta stack
                        opts_this_pass += 1
                        continue

                # reorder copying of 32-bit value to preg if it looks as if that could
                # allow further optimization
                # looking for
                #   lda something
                #   sta.b tcc_rX
                #   lda something
                #   sta.b tcc_rYh
                #   ...tcc_rX...
                if text[i].startswith('lda') and text[i+1].startswith('sta.b tcc__r'):
                    reg = text[i+1][6:]
                    if not reg.endswith('h') and \
                            text[i+2].startswith('lda') and not text[i+2].endswith(reg) and \
                            text[i+3].startswith('sta.b tcc__r') and text[i+3].endswith('h') and \
                            text[i+4].endswith(reg):
                        text_opt += [text[i+2], text[i+3]]
                        text_opt += [text[i], text[i+1]]
                        i += 4
                        # this is not an optimization per se, so we don't count it
                        continue

            elif text[i] == 'rep #$20' and text[i+1] == 'sep #$20':
                i += 2
                opts_this_pass += 1
                continue

            elif text[i] == 'sep #$20' and text[i+1].startswith('lda #') and text[i+2] == 'pha' and text[i+3].startswith('lda #') and text[i+4] == 'pha':
                text_opt += ['pea.w (' + text[i+1].split('#')[1] +
                             ' * 256 + ' + text[i+3].split('#')[1] + ')']
                text_opt += [text[i]]
                i += 5
                opts_this_pass += 1
                continue

            elif text[i][:6] in ['lda.l ', 'sta.l ']:
                cont = False
                for b in bss:
                    if text[i][2:].startswith('a.l ' + b + ' '):
                        text_opt += [text[i].replace('lda.l',
                                                     'lda.w').replace('sta.l', 'sta.w')]
                        i += 1
                        opts_this_pass += 1
                        cont = True
                        break
                if cont:
                    continue

            elif text[i].startswith('jmp.w ') or text[i].startswith('bra __'):
                j = i + 1
                cont = False
                while j < len(text) and text[j].endswith(':'):
                    if text[i].endswith(text[j][:-1]):
                        # redundant branch, discard it
                        i += 1
                        opts_this_pass += 1
                        cont = True
                        break
                    j += 1
                if cont:
                    continue

            elif text[i].startswith('jmp.w '):
                # worst case is a 4-byte instruction, so if the jump target is closer
                # than 32 instructions, we can safely substitute a branch
                label = text[i][6:] + ':'
                cont = False
                for lpos in range(max(0, i - 32), min(len(text), i + 32)):
                    if text[lpos] == label:
                        text_opt += [text[i].replace('jmp.w', 'bra')]
                        i += 1
                        opts_this_pass += 1
                        cont = True
                        break
                if cont:
                    continue

            else:
                r = re.match('adc #(.*)$', text[i])
                if r:
                    r1 = re.match('sta.b (tcc__[fr][0-9]*)$', text[i+1])
                    if r1:
                        if text[i+2] == 'inc.b ' + r1.groups()[0] and text[i+3] == 'inc.b ' + r1.groups()[0]:
                            text_opt += ['adc #' + r.groups()[0] + ' + 2']
                            text_opt += [text[i+1]]
                            i += 4
                            opts_this_pass += 1
                            continue

            text_opt += [text[i]]
            i += 1
        text = text_opt
        if verbose:
            print("{0} optimizations performed\n".format(opts_this_pass))
        opts_total += opts_this_pass

    if verbose:
        for l in text_opt:
            print(l)
        sys.stderr.write(str(opts_total) +
                         ' optimizations performed in total\n')


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="65816 assembly optimizer")
    parser.add_argument("input_file", type=argparse.FileType(),
                        help="the file to optimize")
    parser.add_argument("-o", "--output-file", required=False, type=argparse.FileType("w"),
                        help="if specified, the file path to write the optimized results to; otherwise [input-file]-opt.s")
    parser.add_argument("-v", "--verbose", action="store_true",
                        required=False, help="if specified, verbose output")
    args = parser.parse_args()
    main(args.input_file, args.output_file, args.verbose)
