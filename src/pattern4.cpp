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
    int GetLoopNumber(){
        int j = total_element;
        int loop = 0;

        while (j>0){
            j = j/2;
            loop++;
        }

        return loop;
    }

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
                int j = total_element;
                std::string def_code = "";
                for (int i=1;i<loop_number;i++){
                    def_code += target_type+" window"+to_string(i)+"["+to_string(j/2)+"] = {0};\n";
                    j = j/2;
                }

                std::string loop_code = "";
                j = total_element;
                loop_code += "L1:for(ap_uint<"+to_string(loop_number)+"> x=0; x.to_uint()<"+to_string(j/2)+";x++){\n";
                loop_code += "  #pragma HLS PIPELINE\n";
                loop_code += "  window1[x] = "+target_varible+"[x]+"+target_varible+"["+to_string(j/2)+"+x];\n}\n";
                j = j/2;

                for (int i=2;i<loop_number;i++){
                    loop_code += "L"+to_string(i)+":for(ap_uint<"+to_string(loop_number)+"> x=0; x.to_uint()<"+to_string(j/2)+";x++){\n";
                    loop_code += "  #pragma HLS PIPELINE\n";
                    loop_code += "  window"+to_string(i)+"[x] = window"+to_string(i-1)+"[x]+window"+to_string(i-1)+"["+to_string(j/2)+"+x];\n}\n";
                    j=j/2;
                }

                TheRewriter.InsertText(s->getBeginLoc(),def_code+loop_code);
        }

        //if (delete_content==TheRewriter.getRewrittenText(s->getSourceRange())) {
        //    TheRewriter.RemoveText(s->getSourceRange());
        //}

        return true;
    }

    bool setArgument(std::string location_str){    
        location = location_str;
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
    std::string location=location_str;
    int total_element = 32;
    int loop_number = GetLoopNumber();
    std::string target_varible = "b";
    std::string target_type = "";
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


