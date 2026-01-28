; RUN: llc -mtriple=m65832 < %s | FileCheck %s

; Test zero-extended byte load
define i32 @load_zext_i8(ptr %p) {
; CHECK-LABEL: load_zext_i8:
; CHECK: LDA
  %v = load i8, ptr %p
  %ext = zext i8 %v to i32
  ret i32 %ext
}

; Test sign-extended byte load
define i32 @load_sext_i8(ptr %p) {
; CHECK-LABEL: load_sext_i8:
  %v = load i8, ptr %p
  %ext = sext i8 %v to i32
  ret i32 %ext
}

; Test zero-extended 16-bit load
define i32 @load_zext_i16(ptr %p) {
; CHECK-LABEL: load_zext_i16:
  %v = load i16, ptr %p
  %ext = zext i16 %v to i32
  ret i32 %ext
}

; Test sign-extended 16-bit load  
define i32 @load_sext_i16(ptr %p) {
; CHECK-LABEL: load_sext_i16:
  %v = load i16, ptr %p
  %ext = sext i16 %v to i32
  ret i32 %ext
}
