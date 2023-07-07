; ModuleID = 'bugs/bug_1/dekker_weaker_2.bc'
source_filename = "../c/pthread-atomic/dekker.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%union.pthread_attr_t = type { i64, [48 x i8] }

@flag1 = dso_local global i32 0, align 4
@flag2 = dso_local global i32 0, align 4
@turn = common dso_local global i32 0, align 4
@x = common dso_local global i32 0, align 4

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i8* @thr1(i8* %arg) #0 {
_2:
  %_store = alloca i8*, align 8
  store i8* %arg, i8** %_store, align 8
  store i32 1, i32* @flag1, align 4
  br label %_4

_4:                                               ; preds = %_br6, %_2
  %_5 = load i32, i32* @flag2, align 4
  %_br = icmp sle i32 1, %_5
  br i1 %_br, label %_7, label %_store7

_7:                                               ; preds = %_4
  call void @verifier.assume(i1 true)
  %assert_ = icmp eq i32 %_5, 53
  call void @__VERIFIER_assert(i1 %assert_)
  %oracle_ = sub i32 0, %_5
  %oracle_12 = icmp sle i32 %oracle_, -1
  call void @__VERIFIER_assert(i1 %oracle_12)
  %_8 = load i32, i32* @turn, align 4
  %_br1 = icmp ne i32 %_8, 0
  br i1 %_br1, label %_store2, label %_br6

_store2:                                          ; preds = %_7
  %crab_ = sub i32 0, %_5
  %crab_119 = icmp sle i32 %crab_, -1
  call void @verifier.assume(i1 %crab_119)
  %oracle_15 = sub i32 0, %_5
  %oracle_16 = icmp sle i32 %oracle_15, -1
  call void @__VERIFIER_assert(i1 %oracle_16)
  store i32 0, i32* @flag1, align 4
  br label %_11

_11:                                              ; preds = %_br4, %_store2
  %crab_120 = sub i32 0, %_5
  %crab_121 = icmp sle i32 %crab_120, -1
  call void @verifier.assume(i1 %crab_121)
  %oracle_19 = sub i32 0, %_5
  %oracle_20 = icmp sle i32 %oracle_19, -1
  call void @__VERIFIER_assert(i1 %oracle_20)
  %_12 = load i32, i32* @turn, align 4
  %_br3 = icmp ne i32 %_12, 0
  br i1 %_br3, label %_br4, label %_store5

_br4:                                             ; preds = %_11
  %crab_122 = sub i32 0, %_5
  %crab_123 = icmp sle i32 %crab_122, -1
  call void @verifier.assume(i1 %crab_123)
  %oracle_23 = sub i32 0, %_5
  %oracle_24 = icmp sle i32 %oracle_23, -1
  call void @__VERIFIER_assert(i1 %oracle_24)
  br label %_11

_store5:                                          ; preds = %_11
  %crab_124 = sub i32 0, %_5
  %crab_125 = icmp sle i32 %crab_124, 9
  call void @verifier.assume(i1 %crab_125)
  %crab_126 = sub i32 0, %_12
  %crab_127 = icmp sle i32 %crab_126, 0
  call void @verifier.assume(i1 %crab_127)
  %crab_128 = add i32 0, %_12
  %crab_129 = icmp sle i32 %crab_128, 0
  call void @verifier.assume(i1 %crab_129)
  %oracle_27 = sub i32 0, %_5
  %oracle_28 = icmp sle i32 %oracle_27, -1
  call void @__VERIFIER_assert(i1 %oracle_28)
  %assert_31 = icmp eq i32 %_12, 30
  call void @__VERIFIER_assert(i1 %assert_31)
  %oracle_32 = sub i32 0, %_12
  %oracle_33 = icmp sle i32 %oracle_32, 0
  call void @__VERIFIER_assert(i1 %oracle_33)
  %oracle_36 = add i32 0, %_12
  %oracle_37 = icmp sle i32 %oracle_36, 0
  call void @__VERIFIER_assert(i1 %oracle_37)
  store i32 1, i32* @flag1, align 4
  br label %_br6

_br6:                                             ; preds = %_store5, %_7
  %crab_130 = sub i32 0, %_5
  %crab_131 = icmp sle i32 %crab_130, 0
  call void @verifier.assume(i1 %crab_131)
  %oracle_40 = sub i32 0, %_5
  %oracle_41 = icmp sle i32 %oracle_40, -1
  call void @__VERIFIER_assert(i1 %oracle_41)
  br label %_4

_store7:                                          ; preds = %_4
  call void @verifier.assume(i1 true)
  %oracle_44 = add i32 0, %_5
  %oracle_45 = icmp sle i32 %oracle_44, 0
  call void @__VERIFIER_assert(i1 %oracle_45)
  store i32 0, i32* @x, align 4
  %_18 = load i32, i32* @x, align 4
  %_br8 = icmp sle i32 %_18, 0
  br i1 %_br8, label %_store10, label %_br9

_br9:                                             ; preds = %_store7
  %crab_132 = add i32 0, %_5
  %crab_133 = icmp sle i32 %crab_132, 0
  call void @verifier.assume(i1 %crab_133)
  %crab_134 = sub i32 0, %_18
  %crab_135 = icmp sle i32 %crab_134, -1
  call void @verifier.assume(i1 %crab_135)
  %assert_48 = icmp eq i32 %_5, 30
  call void @__VERIFIER_assert(i1 %assert_48)
  %oracle_49 = add i32 0, %_5
  %oracle_50 = icmp sle i32 %oracle_49, 0
  call void @__VERIFIER_assert(i1 %oracle_50)
  %assert_53 = icmp eq i32 %_18, 80
  call void @__VERIFIER_assert(i1 %assert_53)
  %oracle_54 = sub i32 0, %_18
  %oracle_55 = icmp sle i32 %oracle_54, -1
  call void @__VERIFIER_assert(i1 %oracle_55)
  br label %_call

_call:                                            ; preds = %_br9
  call void (...) @__VERIFIER_error() #5
  unreachable

_store10:                                         ; preds = %_store7
  store i32 1, i32* @turn, align 4
  store i32 0, i32* @flag1, align 4
  ret i8* null
}

; Function Attrs: noreturn
declare dso_local void @__VERIFIER_error(...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i8* @thr2(i8* %arg) #0 {
_2:
  %_store = alloca i8*, align 8
  store i8* %arg, i8** %_store, align 8
  store i32 1, i32* @flag2, align 4
  br label %_4

_4:                                               ; preds = %_br6, %_2
  %_5 = load i32, i32* @flag1, align 4
  %_br = icmp sle i32 1, %_5
  br i1 %_br, label %_7, label %_store7

_7:                                               ; preds = %_4
  call void @verifier.assume(i1 true)
  %oracle_ = sub i32 0, %_5
  %oracle_12 = icmp sle i32 %oracle_, -1
  call void @__VERIFIER_assert(i1 %oracle_12)
  %_8 = load i32, i32* @turn, align 4
  %_br1 = icmp ne i32 %_8, 1
  br i1 %_br1, label %_store2, label %_br6

_store2:                                          ; preds = %_7
  %crab_ = sub i32 0, %_5
  %crab_113 = icmp sle i32 %crab_, -1
  call void @verifier.assume(i1 %crab_113)
  %assert_ = icmp eq i32 %_5, 56
  call void @__VERIFIER_assert(i1 %assert_)
  %oracle_15 = sub i32 0, %_5
  %oracle_16 = icmp sle i32 %oracle_15, -1
  call void @__VERIFIER_assert(i1 %oracle_16)
  store i32 0, i32* @flag2, align 4
  br label %_11

_11:                                              ; preds = %_br4, %_store2
  call void @verifier.assume(i1 true)
  %oracle_19 = sub i32 0, %_5
  %oracle_20 = icmp sle i32 %oracle_19, -1
  call void @__VERIFIER_assert(i1 %oracle_20)
  %_12 = load i32, i32* @turn, align 4
  %_br3 = icmp ne i32 %_12, 1
  br i1 %_br3, label %_br4, label %_store5

_br4:                                             ; preds = %_11
  call void @verifier.assume(i1 true)
  %oracle_23 = sub i32 0, %_5
  %oracle_24 = icmp sle i32 %oracle_23, -1
  call void @__VERIFIER_assert(i1 %oracle_24)
  br label %_11

_store5:                                          ; preds = %_11
  %crab_114 = sub i32 0, %_5
  %crab_115 = icmp sle i32 %crab_114, -1
  call void @verifier.assume(i1 %crab_115)
  %crab_116 = sub i32 0, %_12
  %crab_117 = icmp sle i32 %crab_116, 2
  call void @verifier.assume(i1 %crab_117)
  %crab_118 = add i32 0, %_12
  %crab_119 = icmp sle i32 %crab_118, 2
  call void @verifier.assume(i1 %crab_119)
  %oracle_27 = sub i32 0, %_5
  %oracle_28 = icmp sle i32 %oracle_27, -1
  call void @__VERIFIER_assert(i1 %oracle_28)
  %oracle_31 = sub i32 0, %_12
  %oracle_32 = icmp sle i32 %oracle_31, -1
  call void @__VERIFIER_assert(i1 %oracle_32)
  %assert_35 = icmp eq i32 %_12, 6
  call void @__VERIFIER_assert(i1 %assert_35)
  %oracle_36 = add i32 0, %_12
  %oracle_37 = icmp sle i32 %oracle_36, 1
  call void @__VERIFIER_assert(i1 %oracle_37)
  store i32 1, i32* @flag2, align 4
  br label %_br6

_br6:                                             ; preds = %_store5, %_7
  %crab_120 = sub i32 0, %_5
  %crab_121 = icmp sle i32 %crab_120, -1
  call void @verifier.assume(i1 %crab_121)
  %oracle_40 = sub i32 0, %_5
  %oracle_41 = icmp sle i32 %oracle_40, -1
  call void @__VERIFIER_assert(i1 %oracle_41)
  br label %_4

_store7:                                          ; preds = %_4
  call void @verifier.assume(i1 true)
  %assert_44 = icmp eq i32 %_5, 66
  call void @__VERIFIER_assert(i1 %assert_44)
  %oracle_45 = add i32 0, %_5
  %oracle_46 = icmp sle i32 %oracle_45, 0
  call void @__VERIFIER_assert(i1 %oracle_46)
  store i32 1, i32* @x, align 4
  %_18 = load i32, i32* @x, align 4
  %_br8 = icmp sle i32 1, %_18
  br i1 %_br8, label %_store10, label %_br9

_br9:                                             ; preds = %_store7
  %crab_122 = add i32 0, %_5
  %crab_123 = icmp sle i32 %crab_122, 0
  call void @verifier.assume(i1 %crab_123)
  %crab_124 = add i32 0, %_18
  %crab_125 = icmp sle i32 %crab_124, 3
  call void @verifier.assume(i1 %crab_125)
  %oracle_49 = add i32 0, %_5
  %oracle_50 = icmp sle i32 %oracle_49, 0
  call void @__VERIFIER_assert(i1 %oracle_50)
  %assert_53 = icmp eq i32 %_18, 79
  call void @__VERIFIER_assert(i1 %assert_53)
  %oracle_54 = add i32 0, %_18
  %oracle_55 = icmp sle i32 %oracle_54, 0
  call void @__VERIFIER_assert(i1 %oracle_55)
  br label %_call

_call:                                            ; preds = %_br9
  call void (...) @__VERIFIER_error() #5
  unreachable

_store10:                                         ; preds = %_store7
  store i32 1, i32* @turn, align 4
  store i32 0, i32* @flag2, align 4
  ret i8* null
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
_1:
  %_2 = alloca i32, align 4
  %_3 = alloca i64, align 8
  %_store = alloca i64, align 8
  store i32 0, i32* %_2, align 4
  %_5 = load i32, i32* @turn, align 4
  %_br = icmp sle i32 0, %_5
  br label %_7

_7:                                               ; preds = %_1
  %crab_ = sub i32 0, %_5
  %crab_17 = icmp sle i32 %crab_, 9
  call void @verifier.assume(i1 %crab_17)
  call void @verifier.assume(i1 true)
  %oracle_ = sub i32 0, %_5
  %oracle_3 = icmp sle i32 %oracle_, 0
  call void @__VERIFIER_assert(i1 %oracle_3)
  %assert_ = icmp eq i32 %_5, 37
  call void @__VERIFIER_assert(i1 %assert_)
  %oracle_6 = add i32 0, %_5
  %oracle_7 = icmp sle i32 %oracle_6, 0
  call void @__VERIFIER_assert(i1 %oracle_7)
  %_8 = load i32, i32* @turn, align 4
  %_br1 = icmp sle i32 %_8, 1
  br label %_10

_10:                                              ; preds = %_7
  %_11 = phi i1 [ %_br1, %_7 ]
  %_call = zext i1 %_11 to i32
  call void @__VERIFIER_assume(i32 %_call)
  %_13 = call i32 @pthread_create(i64* %_3, %union.pthread_attr_t* null, i8* (i8*)* @thr1, i8* null) #6
  %_14 = call i32 @pthread_create(i64* %_store, %union.pthread_attr_t* null, i8* (i8*)* @thr2, i8* null) #6
  %_15 = load i64, i64* %_3, align 8
  %_16 = call i32 @pthread_join(i64 %_15, i8** null)
  %_17 = load i64, i64* %_store, align 8
  %_ret = call i32 @pthread_join(i64 %_17, i8** null)
  ret i32 0
}

declare dso_local void @__VERIFIER_assume(i32) #2

; Function Attrs: nounwind
declare !callback !2 dso_local i32 @pthread_create(i64*, %union.pthread_attr_t*, i8* (i8*)*, i8*) #3

declare dso_local i32 @pthread_join(i64, i8**) #2

; Function Attrs: inaccessiblememonly norecurse nounwind optnone
declare void @verifier.assume(i1) #4

declare void @__VERIFIER_assert(i1)

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { noreturn "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #4 = { inaccessiblememonly norecurse nounwind optnone }
attributes #5 = { noreturn }
attributes #6 = { nounwind }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"Ubuntu clang version 10.0.1-++20201112101950+ef32c611aa2-1~exp1~20201112092551.202"}
!2 = !{!3}
!3 = !{i64 2, i64 3, i1 false}
