//===--------------- WMMAAPIMigration.cpp -----------------------------===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===------------------------------------------------------------------===//

#include "RulesLang/WMMAAPIMigration.h"
#include "RuleInfra/ExprAnalysis.h"

using namespace clang::dpct;
using namespace clang::ast_matchers;

void clang::dpct::WMMARule::registerMatcher(ast_matchers::MatchFinder &MF) {
  auto FuncName = []() {
    return hasAnyName("fill_fragment", "load_matrix_sync", "mma_sync",
                      "store_matrix_sync");
  };
  MF.addMatcher(
      typeLoc(loc(qualType(hasDeclaration(namedDecl(allOf(
                  hasAnyName("fragment", "matrix_a", "matrix_b", "row_major",
                             "col_major", "accumulator", "layout_t"),
                  hasDeclContext(namespaceDecl(hasName("wmma")))))))))
          .bind("type"),
      this);
  MF.addMatcher(callExpr(anyOf(callee(functionDecl(allOf(
                                   FuncName(), hasDeclContext(namespaceDecl(
                                                   hasName("wmma")))))),
                               callee(unresolvedLookupExpr(
                                   hasAnyDeclaration(namedDecl(FuncName()))))))
                    .bind("call"),
                this);
  MF.addMatcher(declRefExpr(to(enumConstantDecl(allOf(
                                hasAnyName("mem_row_major", "mem_col_major"),
                                hasType(enumDecl(hasDeclContext(
                                    namespaceDecl(hasName("wmma")))))))))
                    .bind("enum"),
                this);
}

void clang::dpct::WMMARule::runRule(
    const ast_matchers::MatchFinder::MatchResult &Result) {
  ExprAnalysis EA;
  if (auto TL = getNodeAsType<TypeLoc>(Result, "type")) {
    if (DpctGlobalInfo::useSYCLCompat()) {
      if (auto TD = TL->getType()->getAsTagDecl()) {
        report(TL->getBeginLoc(), Diagnostics::UNSUPPORT_SYCLCOMPAT, false,
               TD->getQualifiedNameAsString());
      }
      return;
    }
    EA.analyze(*TL);
  } else if (const CallExpr *CE = getNodeAsType<CallExpr>(Result, "call")) {
    EA.analyze(CE);
  } else if (const DeclRefExpr *DRE =
                 getNodeAsType<DeclRefExpr>(Result, "enum")) {
    if (DpctGlobalInfo::useSYCLCompat()) {
      return;
    }
    EA.analyze(DRE);
  } else {
    return;
  }
  emplaceTransformation(EA.getReplacement());
  EA.applyAllSubExprRepl();
}
