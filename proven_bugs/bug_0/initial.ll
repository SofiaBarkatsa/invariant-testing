; ModuleID = 'bugs/bug_0/initial.bc'
source_filename = "../c/list-simple/sll2c_remove_all.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.node = type { %struct.node*, i32 }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @myexit(i32 %arg) #0 {
_2:
  %_store = alloca i32, align 4
  store i32 %arg, i32* %_store, align 4
  br label %_br

_br:                                              ; preds = %_br, %_2
  br label %_br
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local %struct.node* @node_create(i32 %arg) #0 {
_2:
  %_3 = alloca i32, align 4
  %_store = alloca %struct.node*, align 8
  store i32 %arg, i32* %_3, align 4
  %_5 = call noalias i8* @malloc(i64 16) #4
  %_store1 = bitcast i8* %_5 to %struct.node*
  store %struct.node* %_store1, %struct.node** %_store, align 8
  %_7 = load %struct.node*, %struct.node** %_store, align 8
  %_br = icmp eq %struct.node* null, %_7
  br i1 %_br, label %_call, label %_10

_call:                                            ; preds = %_2
  call void @myexit(i32 1)
  br label %_10

_10:                                              ; preds = %_call, %_2
  %_11 = load %struct.node*, %struct.node** %_store, align 8
  %_store2 = getelementptr inbounds %struct.node, %struct.node* %_11, i32 0, i32 0
  store %struct.node* null, %struct.node** %_store2, align 8
  %_13 = load i32, i32* %_3, align 4
  %_14 = load %struct.node*, %struct.node** %_store, align 8
  %_store3 = getelementptr inbounds %struct.node, %struct.node* %_14, i32 0, i32 1
  store i32 %_13, i32* %_store3, align 8
  %_ret = load %struct.node*, %struct.node** %_store, align 8
  ret %struct.node* %_ret
}

; Function Attrs: nounwind
declare dso_local noalias i8* @malloc(i64) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local %struct.node* @sll_circular_create(i32 %arg, i32 %arg1) #0 {
_3:
  %_4 = alloca i32, align 4
  %_5 = alloca i32, align 4
  %_6 = alloca %struct.node*, align 8
  %_7 = alloca %struct.node*, align 8
  %_store = alloca %struct.node*, align 8
  store i32 %arg, i32* %_4, align 4
  store i32 %arg1, i32* %_5, align 4
  %_9 = load i32, i32* %_5, align 4
  %_store2 = call %struct.node* @node_create(i32 %_9)
  store %struct.node* %_store2, %struct.node** %_6, align 8
  %_store3 = load %struct.node*, %struct.node** %_6, align 8
  store %struct.node* %_store3, %struct.node** %_7, align 8
  br label %_12

_12:                                              ; preds = %_15, %_3
  %crab_ = sub i32 0, %arg1
  %crab_46 = add i32 %crab_, %_9
  %crab_47 = icmp sle i32 %crab_46, 0
  call void @verifier.assume(i1 %crab_47)
  %crab_48 = add i32 0, %arg1
  %crab_49 = sub i32 %crab_48, %_9
  %crab_50 = icmp sle i32 %crab_49, 0
  call void @verifier.assume(i1 %crab_50)
  %assert_ = icmp eq i32 %arg1, 53
  %0 = zext i1 %assert_ to i32
  call void @crab.assert(i32 %0)
  %oracle_ = sub i32 0, %arg1
  %oracle_11 = add i32 %oracle_, %_9
  %oracle_12 = icmp sle i32 %oracle_11, 0
  %1 = zext i1 %oracle_12 to i32
  call void @crab.assert(i32 %1)
  %oracle_16 = add i32 0, %arg1
  %oracle_17 = sub i32 %oracle_16, %_9
  %oracle_18 = icmp sle i32 %oracle_17, 0
  %2 = zext i1 %oracle_18 to i32
  call void @crab.assert(i32 %2)
  %_13 = load i32, i32* %_4, align 4
  %_br = icmp slt i32 1, %_13
  br i1 %_br, label %_15, label %_24

_15:                                              ; preds = %_12
  %crab_51 = sub i32 0, %arg
  %crab_52 = icmp sle i32 %crab_51, -2
  call void @verifier.assume(i1 %crab_52)
  %crab_53 = sub i32 0, %_13
  %crab_54 = icmp sle i32 %crab_53, -2
  call void @verifier.assume(i1 %crab_54)
  %crab_55 = sub i32 0, %arg
  %crab_56 = add i32 %crab_55, %_13
  %crab_57 = icmp sle i32 %crab_56, 0
  call void @verifier.assume(i1 %crab_57)
  %crab_58 = sub i32 0, %arg1
  %crab_59 = add i32 %crab_58, %_9
  %crab_60 = icmp sle i32 %crab_59, 0
  call void @verifier.assume(i1 %crab_60)
  %crab_61 = add i32 0, %arg1
  %crab_62 = sub i32 %crab_61, %_9
  %crab_63 = icmp sle i32 %crab_62, 0
  call void @verifier.assume(i1 %crab_63)
  %oracle_21 = sub i32 0, %arg
  %oracle_22 = icmp sle i32 %oracle_21, -2
  %3 = zext i1 %oracle_22 to i32
  call void @crab.assert(i32 %3)
  %oracle_25 = sub i32 0, %_13
  %oracle_26 = icmp sle i32 %oracle_25, -2
  %4 = zext i1 %oracle_26 to i32
  call void @crab.assert(i32 %4)
  %oracle_30 = sub i32 0, %arg
  %oracle_31 = add i32 %oracle_30, %_13
  %oracle_32 = icmp sle i32 %oracle_31, 0
  %5 = zext i1 %oracle_32 to i32
  call void @crab.assert(i32 %5)
  %assert_36 = icmp eq i32 %arg1, 30
  %6 = zext i1 %assert_36 to i32
  call void @crab.assert(i32 %6)
  %oracle_37 = sub i32 0, %arg1
  %oracle_38 = add i32 %oracle_37, %_9
  %oracle_39 = icmp sle i32 %oracle_38, 0
  %7 = zext i1 %oracle_39 to i32
  call void @crab.assert(i32 %7)
  %oracle_43 = add i32 0, %arg1
  %oracle_44 = sub i32 %oracle_43, %_9
  %oracle_45 = icmp sle i32 %oracle_44, 0
  %8 = zext i1 %oracle_45 to i32
  call void @crab.assert(i32 %8)
  %_16 = load i32, i32* %_5, align 4
  %_store4 = call %struct.node* @node_create(i32 %_16)
  store %struct.node* %_store4, %struct.node** %_store, align 8
  %_18 = load %struct.node*, %struct.node** %_6, align 8
  %_19 = load %struct.node*, %struct.node** %_store, align 8
  %_store5 = getelementptr inbounds %struct.node, %struct.node* %_19, i32 0, i32 0
  store %struct.node* %_18, %struct.node** %_store5, align 8
  %_store6 = load %struct.node*, %struct.node** %_store, align 8
  store %struct.node* %_store6, %struct.node** %_6, align 8
  %_22 = load i32, i32* %_4, align 4
  %_store7 = add nsw i32 %_22, -1
  store i32 %_store7, i32* %_4, align 4
  br label %_12

_24:                                              ; preds = %_12
  %_25 = load %struct.node*, %struct.node** %_6, align 8
  %_26 = load %struct.node*, %struct.node** %_7, align 8
  %_store8 = getelementptr inbounds %struct.node, %struct.node* %_26, i32 0, i32 0
  store %struct.node* %_25, %struct.node** %_store8, align 8
  %_ret = load %struct.node*, %struct.node** %_6, align 8
  ret %struct.node* %_ret
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @sll_circular_remove_first(%struct.node** %arg) #0 {
_2:
  %_3 = alloca %struct.node**, align 8
  %_4 = alloca %struct.node*, align 8
  %_store = alloca %struct.node*, align 8
  store %struct.node** %arg, %struct.node*** %_3, align 8
  %_6 = load %struct.node**, %struct.node*** %_3, align 8
  %_7 = load %struct.node*, %struct.node** %_6, align 8
  %_8 = getelementptr inbounds %struct.node, %struct.node* %_7, i32 0, i32 0
  %_store1 = load %struct.node*, %struct.node** %_8, align 8
  store %struct.node* %_store1, %struct.node** %_4, align 8
  %_10 = load %struct.node*, %struct.node** %_4, align 8
  %_11 = load %struct.node**, %struct.node*** %_3, align 8
  %_12 = load %struct.node*, %struct.node** %_11, align 8
  %_br = icmp eq %struct.node* %_10, %_12
  br i1 %_br, label %_14, label %_19

_14:                                              ; preds = %_2
  %_15 = load %struct.node**, %struct.node*** %_3, align 8
  %_16 = load %struct.node*, %struct.node** %_15, align 8
  %_call = bitcast %struct.node* %_16 to i8*
  call void @free(i8* %_call) #4
  %_store2 = load %struct.node**, %struct.node*** %_3, align 8
  store %struct.node* null, %struct.node** %_store2, align 8
  br label %_ret

_19:                                              ; preds = %_2
  %_20 = load %struct.node**, %struct.node*** %_3, align 8
  %_store3 = load %struct.node*, %struct.node** %_20, align 8
  store %struct.node* %_store3, %struct.node** %_store, align 8
  br label %_22

_22:                                              ; preds = %_29, %_19
  %_23 = load %struct.node*, %struct.node** %_store, align 8
  %_24 = getelementptr inbounds %struct.node, %struct.node* %_23, i32 0, i32 0
  %_25 = load %struct.node*, %struct.node** %_24, align 8
  %_26 = load %struct.node**, %struct.node*** %_3, align 8
  %_27 = load %struct.node*, %struct.node** %_26, align 8
  %_br4 = icmp ne %struct.node* %_25, %_27
  br i1 %_br4, label %_29, label %_33

_29:                                              ; preds = %_22
  %_30 = load %struct.node*, %struct.node** %_store, align 8
  %_31 = getelementptr inbounds %struct.node, %struct.node* %_30, i32 0, i32 0
  %_store5 = load %struct.node*, %struct.node** %_31, align 8
  store %struct.node* %_store5, %struct.node** %_store, align 8
  br label %_22

_33:                                              ; preds = %_22
  %_34 = load %struct.node*, %struct.node** %_4, align 8
  %_35 = load %struct.node*, %struct.node** %_store, align 8
  %_store6 = getelementptr inbounds %struct.node, %struct.node* %_35, i32 0, i32 0
  store %struct.node* %_34, %struct.node** %_store6, align 8
  %_37 = load %struct.node**, %struct.node*** %_3, align 8
  %_38 = load %struct.node*, %struct.node** %_37, align 8
  %_call7 = bitcast %struct.node* %_38 to i8*
  call void @free(i8* %_call7) #4
  %_40 = load %struct.node*, %struct.node** %_4, align 8
  %_store8 = load %struct.node**, %struct.node*** %_3, align 8
  store %struct.node* %_40, %struct.node** %_store8, align 8
  br label %_ret

_ret:                                             ; preds = %_33, %_14
  ret void
}

; Function Attrs: nounwind
declare dso_local void @free(i8*) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
_1:
  %_2 = alloca i32, align 4
  %_3 = alloca i32, align 4
  %_4 = alloca i32, align 4
  %_5 = alloca %struct.node*, align 8
  %_store = alloca i32, align 4
  store i32 0, i32* %_2, align 4
  store i32 2, i32* %_3, align 4
  store i32 1, i32* %_4, align 4
  %_store1 = call %struct.node* @sll_circular_create(i32 2, i32 1)
  store %struct.node* %_store1, %struct.node** %_5, align 8
  store i32 0, i32* %_store, align 4
  br label %_8

_8:                                               ; preds = %_call, %_1
  %_9 = load i32, i32* %_store, align 4
  %_br = icmp slt i32 %_9, 2
  br i1 %_br, label %_call, label %_14

_call:                                            ; preds = %_8
  %crab_ = sub i32 0, %_9
  %crab_21 = icmp sle i32 %crab_, 0
  call void @verifier.assume(i1 %crab_21)
  %crab_22 = add i32 0, %_9
  %crab_23 = icmp sle i32 %crab_22, 1
  call void @verifier.assume(i1 %crab_23)
  %oracle_ = sub i32 0, %_9
  %oracle_7 = icmp sle i32 %oracle_, 0
  %0 = zext i1 %oracle_7 to i32
  call void @crab.assert(i32 %0)
  %oracle_10 = add i32 0, %_9
  %oracle_11 = icmp sle i32 %oracle_10, 1
  %1 = zext i1 %oracle_11 to i32
  call void @crab.assert(i32 %1)
  call void @sll_circular_remove_first(%struct.node** %_5)
  %_12 = load i32, i32* %_store, align 4
  %_store2 = add nsw i32 %_12, 1
  store i32 %_store2, i32* %_store, align 4
  br label %_8

_14:                                              ; preds = %_8
  %crab_24 = sub i32 0, %_9
  %crab_25 = icmp sle i32 %crab_24, -2
  call void @verifier.assume(i1 %crab_25)
  %assert_ = icmp eq i32 %_9, 30
  %2 = zext i1 %assert_ to i32
  call void @crab.assert(i32 %2)
  %oracle_14 = sub i32 0, %_9
  %oracle_15 = icmp sle i32 %oracle_14, -2
  %3 = zext i1 %oracle_15 to i32
  call void @crab.assert(i32 %3)
  %_15 = load %struct.node*, %struct.node** %_5, align 8
  %_br3 = icmp ne %struct.node* null, %_15
  br i1 %_br3, label %_br4, label %_ret

_br4:                                             ; preds = %_14
  %crab_26 = sub i32 0, %_9
  %crab_27 = icmp sle i32 %crab_26, -2
  call void @verifier.assume(i1 %crab_27)
  %assert_18 = icmp eq i32 %_9, 80
  %4 = zext i1 %assert_18 to i32
  call void @crab.assert(i32 %4)
  %oracle_19 = sub i32 0, %_9
  %oracle_20 = icmp sle i32 %oracle_19, -2
  %5 = zext i1 %oracle_20 to i32
  call void @crab.assert(i32 %5)
  br label %_call5

_ret:                                             ; preds = %_14
  ret i32 0

_call5:                                           ; preds = %_br4
  call void (...) @__VERIFIER_error() #5
  unreachable
}

; Function Attrs: noreturn
declare dso_local void @__VERIFIER_error(...) #2

; Function Attrs: inaccessiblememonly norecurse nounwind optnone
declare void @verifier.assume(i1) #3

declare void @crab.assert(i32)

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { noreturn "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { inaccessiblememonly norecurse nounwind optnone }
attributes #4 = { nounwind }
attributes #5 = { noreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"Ubuntu clang version 10.0.1-++20201112101950+ef32c611aa2-1~exp1~20201112092551.202"}
