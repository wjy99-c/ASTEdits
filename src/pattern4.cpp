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

static std::string location_str="";

// By implementing RecursiveASTVisitor, we can specify which AST nodes
// we're interested in by overriding relevant methods.
class MyASTVisitor : public RecursiveASTVisitor<MyASTVisitor>
{
public:
    MyASTVisitor(Rewriter &R)
            : TheRewriter(R)
    {}

    bool VisitStmt(Stmt *s) {
        if (isa<DeclStmt>(s)){
            DeclStmt *del_stmt = cast<DeclStmt>(s);
            Decl *del = del_stmt->getSingleDecl();
            string type_string="";
            string name="";
            if (isa<VarDecl>(del)){
                VarDecl *var_del = cast<VarDecl>(del);
                name = var_del->getNameAsString();
                const Type *type = var_del->getType().getTypePtr();
                if (type->isConstantArrayType()){
                    const ConstantArrayType *Array = cast<ConstantArrayType>(type);
                    type_string = Array->getElementType().getAsString();
                    //maybe we can use Array->getsize to update total_element
                }
                if (name == target_varible){
                    target_type = type_string;
                }
            }
        }
        
        printf("hello,%s\n",TheRewriter.getRewrittenText(s->getSourceRange()).c_str());
        if (location==TheRewriter.getRewrittenText(s->getSourceRange())){
                printf("hello,%s",TheRewriter.getRewrittenText(s->getSourceRange()).c_str());
                
                std::string argument_code = "int SIZE = "+std::to_string(Size)+"\nint BLOCK_SIZE = "+std::to_string(Block_size)+"\n";
                
                std::string pattern_code = "void blockmatmul(hls::stream<blockvec> &Arows, hls::stream<blockvec> &Bcols, blockmat &ABpartial, int it) {\n
                                            #pragma HLS DATAFLOW\n"+argument_code+"
                                            int counter = it % (SIZE/BLOCK_SIZE);\n
                                            static DTYPE A[BLOCK_SIZE][SIZE];\n
                                            if(counter == 0){ //only load the A rows when necessary\n
                                            loadA: for(int i = 0; i < SIZE; i++) {\n
                                                blockvec tempA = Arows.read();\n
                                                for(int j = 0; j < BLOCK_SIZE; j++) {\n
                                                    #pragma HLS PIPELINE II=1\n
                                                    A[j][i] = tempA.a[j];\n
                                                }\n
                                            }\n
                                            }\n
                                            DTYPE AB[BLOCK_SIZE][BLOCK_SIZE] = { 0 };\n
                                            partialsum: for(int k=0; k < SIZE; k++) {\n
                                                blockvec tempB = Bcols.read();\n
                                                for(int i = 0; i < BLOCK_SIZE; i++) {\n
                                                    for(int j = 0; j < BLOCK_SIZE; j++) {\n
                                                        AB[i][j] = AB[i][j] +  A[i][k] * tempB.a[j];\n
                                                    }\n
                                                }\n
                                            }\n
                                            writeoutput: for(int i = 0; i < BLOCK_SIZE; i++) {\n
                                                for(int j = 0; j < BLOCK_SIZE; j++) {\n
                                                    ABpartial.out[i][j] = AB[i][j];\n
                                                }\n
                                            }\n
                                        }\n";
                


                TheRewriter.InsertText(s->getBeginLoc(),pattern_code);
        }

        //if (delete_content==TheRewriter.getRewrittenText(s->getSourceRange())) {
        //    TheRewriter.RemoveText(s->getSourceRange());
        //}

        return true;
    }

private:

    Rewriter &TheRewriter;
    std::string location=location_str;
    int Size = 3;
    int Block_size= 3;
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
    if (argc < 2) {
        llvm::errs() << "Usage: delete_code <filename>\n";
        return 1;
    }
    location_str = argv[2];

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


