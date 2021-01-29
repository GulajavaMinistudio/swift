// RUN: %empty-directory(%t)
// RUN: %target-swift-frontend -emit-module -enable-experimental-concurrency -enable-library-evolution -emit-module-path=%t/resilient_class.swiftmodule -module-name=resilient_class %S/Inputs/resilient_class.swift
// RUN: %target-swift-frontend -I %t -emit-ir -enable-experimental-concurrency -enable-library-evolution %s | %FileCheck --check-prefix=CHECK --check-prefix=CHECK-%target-cpu %s
// REQUIRES: concurrency

import resilient_class

open class MyBaseClass<T> {
  var value: T

  open func wait() async -> T {
    return value
  }

  open func waitThrows() async throws -> T {
    return value
  }

  // FIXME
  // open func waitGeneric<T>(_: T) async -> T
  // open func waitGenericThrows<T>(_: T) async throws -> T

  public init(_ value: T) {
    self.value = value
  }
}

// CHECK-LABEL: @"$s16class_resilience11MyBaseClassC4waitxyYFTjTu" = {{(dllexport )?}}{{(protected )?}}global %swift.async_func_pointer

// CHECK-LABEL: define {{(dllexport )?}}{{(protected )?}}swiftcc void @"$s16class_resilience11MyBaseClassC4waitxyYFTj"(%swift.task* %0, %swift.executor* %1, %swift.context* swiftasync %2) #0 {
