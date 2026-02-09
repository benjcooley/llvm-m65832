; RUN: llc -mtriple=m65832-elf -O1 -max-store-memcpy=8 -max-store-memmove=8 < %s | FileCheck %s
; CHECK-NOT: @<noreg>
; CHECK: LD.B {{R[0-9]+}},$02

target datalayout = "e-m:e-p:32:32-i8:8-i16:16-i32:32-i64:32:64-f32:32-f64:32:64-n32-S32"
target triple = "m65832-unknown-unknown-elf"

@.str = private unnamed_addr constant [9 x i8] c"abcdefgh\00", align 1

define i32 @main() local_unnamed_addr {
  %buf = alloca [9 x i8], align 1
  call void @llvm.memcpy.p0.p0.i32(ptr align 1 %buf, ptr align 1 @.str, i32 9, i1 false)
  call void asm sideeffect "", "r,~{memory}"(ptr %buf)
  %gep = getelementptr inbounds i8, ptr %buf, i32 2
  %v = load i8, ptr %gep, align 1
  %ext = zext i8 %v to i32
  ret i32 %ext
}

declare void @llvm.memcpy.p0.p0.i32(ptr noalias writeonly, ptr noalias readonly, i32, i1 immarg)
