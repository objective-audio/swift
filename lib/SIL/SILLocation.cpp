//===--- SILLocation.cpp - Location information for SIL nodes -------------===//
//
// This source file is part of the Swift.org open source project
//
// Copyright (c) 2014 - 2015 Apple Inc. and the Swift project authors
// Licensed under Apache License v2.0 with Runtime Library Exception
//
// See http://swift.org/LICENSE.txt for license information
// See http://swift.org/CONTRIBUTORS.txt for the list of Swift project authors
//
//===----------------------------------------------------------------------===//

#include "swift/SIL/SILLocation.h"
#include "swift/AST/Decl.h"
#include "swift/AST/Expr.h"
#include "swift/AST/Pattern.h"
#include "swift/AST/Stmt.h"
#include "llvm/Support/raw_ostream.h"

using namespace swift;

SourceLoc SILLocation::getSourceLoc() const {
  if (ASTNode.isNull())
    return SILFileSourceLoc;

  if (alwaysPointsToStart()) {
    return getStartSourceLoc();
  }

  if (alwaysPointsToEnd()) {
    return getEndSourceLoc();
  }

  // Use the end location for the CleanupKind.
  if (getKind() == CleanupKind) {
    return getEndSourceLoc();
  }

  // Use the end location for the ImplicitReturnKind.
  if (getKind() == ImplicitReturnKind) {
    return getEndSourceLoc();
  }

  // Use the start location for the ReturnKind.
  if (getKind() == ReturnKind) {
    return getStartSourceLoc();
  }

  if (auto decl = ASTNode.dyn_cast<Decl*>())
    return decl->getLoc();
  if (auto expr = ASTNode.dyn_cast<Expr*>())
    return expr->getLoc();
  if (auto stmt = ASTNode.dyn_cast<Stmt*>())
    return stmt->getStartLoc();
  if (auto patt = ASTNode.dyn_cast<Pattern*>())
    return patt->getStartLoc();
  llvm_unreachable("impossible SILLocation");
}

SourceLoc SILLocation::getStartSourceLoc() const {
  if (ASTNode.isNull())
    return SILFileSourceLoc;
  if (auto decl = ASTNode.dyn_cast<Decl*>())
    return decl->getStartLoc();
  if (auto expr = ASTNode.dyn_cast<Expr*>())
    return expr->getStartLoc();
  if (auto stmt = ASTNode.dyn_cast<Stmt*>())
    return stmt->getStartLoc();
  if (auto patt = ASTNode.dyn_cast<Pattern*>())
    return patt->getStartLoc();
  llvm_unreachable("impossible SILLocation");
}

SourceLoc SILLocation::getEndSourceLoc() const {
  if (ASTNode.isNull())
    return SILFileSourceLoc;
  if (auto decl = ASTNode.dyn_cast<Decl*>())
    return decl->getEndLoc();
  if (auto expr = ASTNode.dyn_cast<Expr*>())
    return expr->getEndLoc();
  if (auto stmt = ASTNode.dyn_cast<Stmt*>())
    return stmt->getEndLoc();
  if (auto patt = ASTNode.dyn_cast<Pattern*>())
    return patt->getEndLoc();
  llvm_unreachable("impossible SILLocation");
}

void SILLocation::dump(const SourceManager &SM) const {
  if (auto D = ASTNode.dyn_cast<Decl *>())
    llvm::errs() << Decl::getKindName(D->getKind()) << "Decl @ ";
  if (auto E = ASTNode.dyn_cast<Expr *>())
    llvm::errs() << Expr::getKindName(E->getKind()) << "Expr @ ";
  if (auto S = ASTNode.dyn_cast<Stmt *>())
    llvm::errs() << Stmt::getKindName(S->getKind()) << "Stmt @ ";
  if (auto P = ASTNode.dyn_cast<Pattern *>())
    llvm::errs() << Pattern::getKindName(P->getKind()) << "Pattern @ ";

  print(llvm::errs(), SM);
}

void SILLocation::print(raw_ostream &OS, const SourceManager &SM) const {
  if (isNull())
    OS << "<no loc>";
  getSourceLoc().print(OS, SM);
}

InlinedLocation InlinedLocation::getInlinedLocation(SILLocation L) {
  if (Expr *E = L.getAsASTNode<Expr>())
    return InlinedLocation(E, L.getSpecialFlags());
  if (Stmt *S = L.getAsASTNode<Stmt>())
    return InlinedLocation(S, L.getSpecialFlags());
  if (Pattern *P = L.getAsASTNode<Pattern>())
    return InlinedLocation(P, L.getSpecialFlags());
  if (Decl *D = L.getAsASTNode<Decl>())
    return InlinedLocation(D, L.getSpecialFlags());

  if (Optional<SILFileLocation> FileLoc = L.getAs<SILFileLocation>())
    return InlinedLocation(FileLoc.getValue().getFileLocation(),
                           L.getSpecialFlags());
  // Otherwise, it can be an inlined location wrapping a file location.
  if (Optional<InlinedLocation> InlinedLoc = L.getAs<InlinedLocation>()) {
    return InlinedLocation(InlinedLoc.getValue().getFileLocation(),
                           L.getSpecialFlags());
  }

  llvm_unreachable("Cannot construct Inlined loc from the given location.");
}

MandatoryInlinedLocation
MandatoryInlinedLocation::getMandatoryInlinedLocation(SILLocation L) {
  if (Expr *E = L.getAsASTNode<Expr>())
    return MandatoryInlinedLocation(E, L.getSpecialFlags());
  if (Stmt *S = L.getAsASTNode<Stmt>())
    return MandatoryInlinedLocation(S, L.getSpecialFlags());
  if (Pattern *P = L.getAsASTNode<Pattern>())
    return MandatoryInlinedLocation(P, L.getSpecialFlags());
  if (Decl *D = L.getAsASTNode<Decl>())
    return MandatoryInlinedLocation(D, L.getSpecialFlags());

  if (Optional<SILFileLocation> FileLoc = L.getAs<SILFileLocation>())
    return MandatoryInlinedLocation(FileLoc.getValue().getFileLocation(),
                                    L.getSpecialFlags());
  // Otherwise, it can be an inlined location wrapping a file location.
  if (Optional<MandatoryInlinedLocation> InlinedLoc =
      L.getAs<MandatoryInlinedLocation>()) {
    return MandatoryInlinedLocation(InlinedLoc.getValue().getFileLocation(),
                                    L.getSpecialFlags());
  }

  if (L.isInTopLevel())
    return MandatoryInlinedLocation::getModuleLocation(L.getSpecialFlags());
  
  llvm_unreachable("Cannot construct Inlined loc from the given location.");
}

CleanupLocation CleanupLocation::getCleanupLocation(SILLocation L) {
  if (Expr *E = L.getAsASTNode<Expr>())
    return CleanupLocation(E, L.getSpecialFlags());
  if (Stmt *S = L.getAsASTNode<Stmt>())
    return CleanupLocation(S, L.getSpecialFlags());
  if (Pattern *P = L.getAsASTNode<Pattern>())
    return CleanupLocation(P, L.getSpecialFlags());
  if (Decl *D = L.getAsASTNode<Decl>())
    return CleanupLocation(D, L.getSpecialFlags());
  if (L.isNull())
    return CleanupLocation();
  if (L.getAs<SILFileLocation>())
    return CleanupLocation();
  llvm_unreachable("Cannot construct Cleanup loc from the "
                   "given location.");
}
