//===--- SyntaxData.cpp - Swift Syntax Data Implementation ----------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2017 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See https://swift.org/LICENSE.txt for license information
// See https://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/Syntax/SyntaxData.h"

using namespace swift;
using namespace swift::syntax;

RC<const SyntaxData> SyntaxData::replacingSelf(const RawSyntax *NewRaw) const {
  if (hasParent()) {
    auto NewRoot = getParent()->replacingChild(NewRaw, getIndexInParent());
    auto NewSelf = AbsoluteRaw.replacingSelf(
        NewRaw, NewRoot->AbsoluteRaw.getNodeId().getRootId());
    return RC<const SyntaxData>(new SyntaxData(NewSelf, NewRoot));
  } else {
    auto NewSelf = AbsoluteRawSyntax::forRoot(NewRaw);
    return RC<const SyntaxData>(new SyntaxData(NewSelf));
  }
}

bool SyntaxData::isType() const { return getRaw()->isType(); }

bool SyntaxData::isStmt() const { return getRaw()->isStmt(); }

bool SyntaxData::isDecl() const { return getRaw()->isDecl(); }

bool SyntaxData::isExpr() const { return getRaw()->isExpr(); }

bool SyntaxData::isPattern() const { return getRaw()->isPattern(); }

bool SyntaxData::isUnknown() const { return getRaw()->isUnknown(); }

void SyntaxData::dump(llvm::raw_ostream &OS) const {
  getRaw()->dump(OS, 0);
  OS << '\n';
}

void SyntaxData::dump() const { dump(llvm::errs()); }

RC<const SyntaxData> SyntaxData::getPreviousNode() const {
  if (size_t N = getIndexInParent()) {
    if (hasParent()) {
      for (size_t I = N - 1; ; --I) {
        if (auto C = getParent()->getChild(I)) {
          if (C->getRaw()->isPresent() && C->getFirstToken())
            return C;
        }
        if (I == 0)
          break;
      }
    }
  }
  return hasParent() ? getParent()->getPreviousNode() : nullptr;
}

RC<const SyntaxData> SyntaxData::getNextNode() const {
  if (hasParent()) {
    for (size_t I = getIndexInParent() + 1, N = getParent()->getNumChildren();
         I != N; ++I) {
      if (auto C = getParent()->getChild(I)) {
        if (C->getRaw()->isPresent() && C->getFirstToken())
          return C;
      }
    }
    return getParent()->getNextNode();
  }
  return nullptr;
}

RC<const SyntaxData> SyntaxData::getFirstToken() const {
  if (getRaw()->isToken()) {
    return RC<const SyntaxData>(this);
  }

  for (size_t I = 0, E = getNumChildren(); I < E; ++I) {
    if (auto Child = getChild(I)) {
      if (Child->getRaw()->isMissing())
        continue;
      if (Child->getRaw()->isToken()) {
        return Child;
      } else if (auto Token = Child->getFirstToken()) {
        return Token;
      }
    }
  }
  return nullptr;
}

RC<const SyntaxData> SyntaxData::getLastToken() const {
  if (getRaw()->isToken() && !getRaw()->isMissing()) {
    return RC<const SyntaxData>(this);
  }

  if (getNumChildren() == 0) {
    return nullptr;
  }
  for (int I = getNumChildren() - 1; I >= 0; --I) {
    if (auto Child = getChild(I)) {
      if (Child->getRaw()->isMissing()) {
        continue;
      }
      if (Child->getRaw()->isToken()) {
        return Child;
      } else if (auto Token = Child->getLastToken()) {
        return Token;
      }
    }
  }
  return nullptr;
}

RC<const SyntaxData>
SyntaxData::getChild(AbsoluteSyntaxPosition::IndexInParentType Index) const {
  if (!getRaw()->getChild(Index)) {
    return nullptr;
  }
  /// FIXME: Start from the back (advancedToEndOfChildren) and reverse from
  /// there if Index is closer to the end as a performance improvement?
  AbsoluteSyntaxPosition Position =
      AbsoluteRaw.getInfo().getPosition().advancedToFirstChild();
  SyntaxIdentifier NodeId =
      AbsoluteRaw.getInfo().getNodeId().advancedToFirstChild();

  for (size_t I = 0; I < Index; ++I) {
    Position = Position.advancedBy(getRaw()->getChild(I));
    NodeId = NodeId.advancedBy(getRaw()->getChild(I));
  }
  AbsoluteSyntaxInfo Info(Position, NodeId);

  const RC<const SyntaxData> RefCountedParent = RC<const SyntaxData>(this);
  return RC<const SyntaxData>(new SyntaxData(
      AbsoluteRawSyntax(getRaw()->getChild(Index), Info), RefCountedParent));
}

AbsoluteOffsetPosition
SyntaxData::getAbsolutePositionBeforeLeadingTrivia() const {
  return AbsoluteRaw.getPosition();
}

AbsoluteOffsetPosition
SyntaxData::getAbsolutePositionAfterLeadingTrivia() const {
  if (auto FirstToken = getFirstToken()) {
    return getAbsolutePositionBeforeLeadingTrivia().advancedBy(
        FirstToken->getRaw()->getLeadingTriviaLength());
  } else {
    return getAbsolutePositionBeforeLeadingTrivia();
  }
}

AbsoluteOffsetPosition
SyntaxData::getAbsoluteEndPositionBeforeTrailingTrivia() const {
  if (auto LastToken = getLastToken()) {
    return getAbsoluteEndPositionAfterTrailingTrivia().advancedBy(
        -LastToken->getRaw()->getTrailingTriviaLength());
  } else {
    return getAbsoluteEndPositionAfterTrailingTrivia();
  }
}

AbsoluteOffsetPosition
SyntaxData::getAbsoluteEndPositionAfterTrailingTrivia() const {
  return getAbsolutePositionBeforeLeadingTrivia().advancedBy(
      getRaw()->getTextLength());
}
