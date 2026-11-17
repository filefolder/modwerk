        | SPDX-License-Identifier: GPL-3.0-or-later
        | Pad Thresh's two sites in OS 1.74's pad strike code (0x400821fa), which
        | turns a strike's force byte into the velocity it posts as a type 3 UI
        | event. Neither is a branch target; no other module or the base patches
        | them.
        |
        |   0x40082200 (6)   move.l 0x4028595c,%d1. d0 holds the force byte and
        |                    a0 a table pointer stock uses next: both kept.
        |   0x40082258 (18)  the squared curve: d1 = d1 * d1 * 258 >> 16, d1 the
        |                    force above the threshold scaled to 0-255 (negative
        |                    below it). d2 is free: stock loads it here itself.
        |                    Stock tests d1 next and drops the strike at <= 0.

        .text

        .globl  pad_thresh_force
pad_thresh_force:
        move.b  %d0,pt_last_force
        addq.l  #1,pt_count
        move.l  0x4028595c,%d1          | the replaced instruction
        rts

        .globl  pad_thresh_curve
pad_thresh_curve:
        move.l  pt_curve,%d2
        bne.s   1f
        muls.l  %d1,%d1                 | 0: stock's curve, exactly
        move.l  #258,%d2
        muls.l  %d2,%d1
        swap    %d1
        ext.l   %d1
        bra.s   9f
1:      tst.l   %d1                     | 1 and 2: at or below the threshold plays nothing
        bgt.s   2f
        moveq   #0,%d1
        bra.s   9f
2:      subq.l  #1,%d2
        beq.s   9f                      | 1: linear, the scaled force as it is
        move.l  #510,%d2                | 2: soft, 255 * (2x - x^2) for x = d1 / 255
        sub.l   %d1,%d2
        muls.l  %d2,%d1
        move.l  #258,%d2
        muls.l  %d2,%d1
        swap    %d1
        ext.l   %d1
9:      move.b  %d1,pt_last_vel
        rts
