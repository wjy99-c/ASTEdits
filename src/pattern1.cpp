//------------------------------------------------------------------------------
// Clang rewriter sample. Demonstrates:
//
// * Use RecursiveASTVisitor to find interesting AST nodes.
// * Use the Rewriter API to rewrite the source code.
//------------------------------------------------------------------------------


//TODO: add Par argument number
#include <cstdio>
#include <string>
#include <sstream>

#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Basic/Diagnostic.h"
#include "clang/Basic/FileManager.h"
#include "clang/Basic/SourceManager.h"
#include "clang/Basic/TargetOptions.h"
#include "clang/Basic/TargetInfo.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Lex/Preprocessor.h"
#include "clang/Parse/ParseAST.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Rewrite/Frontend/Rewriters.h"
#include "llvm/Support/Host.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace std;


// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    MyASTVisitor(Rewriter &R)
            : TheRewriter(R)
    {}

    bool VisitStmt(Stmt *s) {
        // Only care about for statements.
        if (isa<ForStmt>(s)) {
            ForStmt *ForStatement = cast<ForStmt>(s);
            Stmt *Cond = ForStatement->getCond();
            Stmt *Operation = ForStatement->getBody();

            TheRewriter.InsertText(Operation->getBeginLoc().getLocWithOffset(1),"#pragma HLS ARRAY_PARTITION variable=f cyclic factor="+to_string(cyclic_factor)+" dim="+to_string(dim)+"\n",true, true);
            //TheRewriter.RemoveText(s->getSourceRange());
            TheRewriter.InsertText(Cond->getBeginLoc(),"#pragma HLS pipeline\n");
        }

        //if (delete_content==TheRewriter.getRewrittenText(s->getSourceRange())) {
        //    TheRewriter.RemoveText(s->getSourceRange());
        //}

        return true;
    }

    bool setArgument(std::string location_str, std::string content){    
        delete_content = content;
        return true;
    }

    bool VisitFunctionDecl(FunctionDecl *f) {
        // Only function definitions (with bodies), not declarations.
        if (f->hasBody()) {
            Stmt *FuncBody = f->getBody();

            // Type name as string
            //QualType QT = f->getResultType();
            QualType QT = f->getReturnType();
            string TypeStr = QT.getAsString();

            // Function name
            DeclarationName DeclName = f->getNameInfo().getName();
            string FuncName = DeclName.getAsString();

            // Add comment before
            stringstream SSBefore;
            SSBefore << "// Begin function " << FuncName << " returning "
                     << TypeStr << "\n";
            SourceLocation ST = f->getSourceRange().getBegin();
            TheRewriter.InsertText(ST, SSBefore.str(), true, true);

            // And after
            stringstream SSAfter;
            SSAfter << "\n// End function " << FuncName << "\n";
            ST = FuncBody->getEndLoc().getLocWithOffset(1);
            TheRewriter.InsertText(ST, SSAfter.str(), true, true);
        }

        return true;
    }

private:

    Rewriter &TheRewriter;
    std::string delete_content="    b[0] = 1;";
    int cyclic_factor = 4;
    int dim = 1;
};


// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser.
class MyASTConsumer : public ASTConsumer
{
public:
    MyASTConsumer(Rewriter &R)
            : Visitor(R)
    {}

    // Override the method that gets called for each parsed top-level
    // declaration.
    virtual bool HandleTopLevelDecl(DeclGroupRef DR) {
        for (DeclGroupRef::iterator b = DR.begin(), e = DR.end();
             b != e; ++b)
            // Traverse the declaration using our AST visitor.
            Visitor.TraverseDecl(*b);
        return true;
    }

private:
    MyASTVisitor Visitor;
};


int main(int argc, char *argv[])
{
    if (argc != 2) {
        llvm::errs() << "Usage: delete_code <filename>\n";
        return 1;
    }

    // CompilerInstance will hold the instance of the Clang compiler for us,
    // managing the various objects needed to run the compiler.
    CompilerInstance TheCompInst;
    //TheCompInst.createDiagnostics(0, 0);
    TheCompInst.createDiagnostics();

    // Initialize target info with the default triple for our platform.
    //TargetOptions TO;
    auto TO = std::make_shared<clang::TargetOptions>();
    TO->Triple = llvm::sys::getDefaultTargetTriple();
    TargetInfo *TI = TargetInfo::CreateTargetInfo(
            TheCompInst.getDiagnostics(), TO);
    TheCompInst.setTarget(TI);

    TheCompInst.createFileManager();
    FileManager &FileMgr = TheCompInst.getFileManager();
    TheCompInst.createSourceManager(FileMgr);
    SourceManager &SourceMgr = TheCompInst.getSourceManager();
    TheCompInst.createPreprocessor(TU_Module);
    TheCompInst.createASTContext();

    // A Rewriter helps us manage the code rewriting task.
    Rewriter TheRewriter;
    TheRewriter.setSourceMgr(SourceMgr, TheCompInst.getLangOpts());

    // Set the main file handled by the source manager to the input file.
    const FileEntry *FileIn = FileMgr.getFile(argv[1]).get();
    //SourceMgr.createMainFileID(FileIn);
    SourceMgr.setMainFileID(
            SourceMgr.createFileID(FileIn, SourceLocation(), SrcMgr::C_User));
    TheCompInst.getDiagnosticClient().BeginSourceFile(
            TheCompInst.getLangOpts(),
            &TheCompInst.getPreprocessor());

    // Create an AST consumer instance which is going to get called by
    // ParseAST.
    MyASTConsumer TheConsumer(TheRewriter);

    // Parse the file to AST, registering our consumer as the AST consumer.
    ParseAST(TheCompInst.getPreprocessor(), &TheConsumer,
             TheCompInst.getASTContext());

    // At this point the rewriter's buffer should be full with the rewritten
    // file contents.
    const RewriteBuffer *RewriteBuf =
            TheRewriter.getRewriteBufferFor(SourceMgr.getMainFileID());
    llvm::outs() << string(RewriteBuf->begin(), RewriteBuf->end());

    return 0;
}


