#include "clang/Driver/Options.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTContext.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/ASTConsumers.h"
#include "clang/Frontend/FrontendActions.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Tooling/CommonOptionsParser.h"
#include "clang/Tooling/Tooling.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/ASTMatchers/ASTMatchers.h"
#include "clang/ASTMatchers/ASTMatchFinder.h"
#include <set>
#include <sstream>
#include <vector>
#include <map>
#include <tuple>
#include <fstream>
#include <iostream>
#include <bitset>

using namespace std;
using namespace clang;
using namespace clang::driver;
using namespace clang::tooling;
using namespace clang::ast_matchers;
using namespace llvm;

/** Options 
 * Custom command line arguments are declared here
 * **/
static cl::OptionCategory CPP2CCategory("CPP2C options");

/** wrap: list of classes to be wrapped.
 * Format of file:
 * cppClassName1
 * cppClassName2
 **/
static cl::opt<std::string> ClassesToBeWrappedArg("wrap", cl::cat(CPP2CCategory));
//TODO: add support for including handwritten code

/** Classes to be mapped to C **/
struct OutputStreams final {
	string headerString; // resulting header code
	string bodyString; // resulting body code

	llvm::raw_string_ostream headerStream;
	llvm::raw_string_ostream bodyStream;

	OutputStreams() : headerString(""), bodyString(""), headerStream(headerString), bodyStream(bodyString) {};
};

/** Resource loading utils **/
class Resources final {
	public:
		Resources(const string fileName);
		void initList();
		const vector<string> getResources() const;
	private:
		vector<string> resources;
		string fileName;
};

Resources::Resources(const string file) : fileName(file) {}

void Resources::initList() {
	ifstream classesFile(fileName);
	string className;

	if (classesFile.is_open()) {
		while (getline(classesFile, className)) {
			resources.push_back(className);
		}
		classesFile.close();
	} else {
		cout << "Error reading file" << endl;
	}
}

const vector<string> Resources::getResources() const {
	return resources;
}

/** Classes to be wrapped **/
Resources* classList;

/** Header to be included in the generated cpp file **/
string headerForSrcFile("");

/** Map with function names used to create unique function signatures in case of overloads **/
map<string, int> funcList;

//TODO: maybe we can just handwrite the code for these cases
std::vector<std::string> smartPtrs {
	"unique_ptr", // std
	"shared_ptr", // std
	"sp", // hwui
	"sk_sp" //skia
	};

//TODO: correctly generate code in these cases
//TODO: generate comments regarding pointer ownership in these cases
int isSmartPtr(const string& name) {
	for (unsigned int i = 0; i < smartPtrs.size(); ++i) {
		const string& ptrName = smartPtrs[i];
		if (name.find(ptrName) != string::npos) {
			return i;
		}
	}

	return -1;
}

class CTypeMetadata {
	const string fCType;
	const string fCastType;
	const bool fIsPointer;
	const bool fIsReference;
	const bool fIsConst;
	const bool fIsVoidType;
	const bool fIsFunctionPointer;
	const string fFunctionPointerNameLeft;
	const string fFunctionPointerNameRight;

	friend class CTypeMetadataConverter;

	CTypeMetadata(const string cType, const string castType, const bool isPointer, const bool isReference, const bool isConst, const bool isVoidType, const bool isFunctionPointer, const string functionPointerNameLeft, const string functionPointerNameRight) 
		: fCType(cType), fCastType(castType), fIsPointer(isPointer), fIsReference(isReference), fIsConst(isConst), fIsVoidType(isVoidType), fIsFunctionPointer(isFunctionPointer),
		fFunctionPointerNameLeft(functionPointerNameLeft), fFunctionPointerNameRight(functionPointerNameRight) {}

public:
	const string getCType() const {
		if (fIsFunctionPointer) {
			return fFunctionPointerNameLeft + fFunctionPointerNameRight;
		} else {
			return fCType;
		}
	}

	const string getCTypeWithName(const string name) const {
		if (fIsFunctionPointer) {
			return string(fFunctionPointerNameLeft) + string(name) + string(fFunctionPointerNameRight);
		} else {
			return fCType + " " + name;
		}
	}

	const string getCastType() const {
		return fCastType;
	}

	const bool isPointer() const {
		return fIsPointer;
	}

	const bool isReference() const {
		return fIsReference;
	}

	const bool isConst() const {
		return fIsConst;
	}

	const bool isVoidType() const {
		return fIsVoidType;
	}
};

/**
 * Constructs an equivalent C type given a C++ type
 */
class CTypeMetadataConverter {
	const clang::ASTContext* context;
	const Resources* const classList;

	string cType = "";
	string castType = ""; //whether this should be casted or not, this is used when a class/struct is wrapped with a C struct
	bool isPointer = false;
	bool isReference = false;
	bool isVoidType = true;
	bool isFunctionPointer = false;

	// used if the type is a function pointer, e.g. void (* foo) (void*, void*)
	// in this case we keep the name as "void(*" + ") (void*, void*)" as we might need to add the variable's name in the middle
	string fpNameLeft = "";
	string fpNameRight = "";

	void reset() {
		cType = "";
		castType = "";
		isPointer = false;
		isReference = false;
		isVoidType = false;
		isFunctionPointer = false;
		fpNameLeft = "";
		fpNameRight = "";
	}

	void wrapClassName(const string& name) {
		const vector<string>& classes = classList->getResources();
		if (std::find(classes.begin(), classes.end(), name) != classes.end()) {
			cType = "W" + name + "*";
			castType = name + "*";
		} else {
			cType = name + "*";
		}
	}

	void getAsEnum(const QualType& qt) {
		cType = qt.getAsString();
	}

	void getAsFunctionPointer(const QualType& qt) {
		const QualType pointee = qt->castAs<clang::PointerType>()->getPointeeType();
		if (pointee->isFunctionProtoType()) {
			isFunctionPointer = true;
			const clang::FunctionProtoType* functionPrototype = pointee->castAs<clang::FunctionProtoType>();

			fpNameLeft = functionPrototype->getReturnType().getAsString() + "(*";
			fpNameRight = ") (";
			for (unsigned int i = 0; i < functionPrototype->getNumParams(); ++i) {
				//TODO: we assume this is a builtin type here, we should recurse instead for each param
				const QualType param = functionPrototype->getParamType(i);
				fpNameRight += (i != 0 ? ", " : "") + param.getAsString();
			}
			fpNameRight += ")";
		}
	}

	void getAsClassTemplateSpecialization(const CXXRecordDecl* crd) {
		auto t = dyn_cast<clang::ClassTemplateSpecializationDecl>(crd);
		const clang::TemplateArgumentList& argList = t->getTemplateArgs();

		const vector<string>& classes = classList->getResources();
		auto name = crd->getNameAsString();

		if (std::find(classes.begin(), classes.end(), name) != classes.end()) {
			cType = "W" + name;
			castType = name + "<";
			for (unsigned int i = 0; i < argList.size(); ++i) {
				const TemplateArgument& arg = argList.get(i);
				PrintingPolicy pp(context->getLangOpts());
				string argName = arg.getAsType().getAsString(pp);

				// TODO find a way to sugar the arg in order to avoid checks like this
				if(argName.find("std::default_delete") != string::npos) {
					continue;
				}

				cType += "_" + argName;
				castType += (i == 0 ? "" : ", ") + argName;
			}
			cType += "*";
			castType += ">";
			//TODO: add definition to header?
		}
	}

	bool isConst(const QualType& qt) {
		return qt.getAsString().find("const ") != string::npos;
	}

public:
	CTypeMetadataConverter(const clang::ASTContext* c, const Resources* cl) : context(c), classList(cl) {}

	CTypeMetadata determineCType(const QualType& qt) {
		reset();

		if (qt->isFunctionPointerType()) {
			getAsFunctionPointer(qt);
		}
		//if it is builtin type, or pointer to builtin, use it as is
		else if(qt->isBuiltinType() || (qt->isPointerType() && qt->getPointeeType()->isBuiltinType())) {
			cType = qt.getAsString();
			if(qt->isVoidType()) {
				isVoidType = true;
			}
			isPointer = true;
		} else if (qt->isEnumeralType()) {
			getAsEnum(qt);
		} else if(qt->isRecordType()) {
			const CXXRecordDecl* crd = qt->getAsCXXRecordDecl();
			string recordName = crd->getQualifiedNameAsString();

			cout << "record type = " + recordName << endl;

			if (isa<clang::ClassTemplateSpecializationDecl>(crd)) {
				//TODO no template support
				//getAsClassTemplateSpecialization(crd);
			} else {
				wrapClassName(recordName);
			}
		} else if((qt->isReferenceType() || qt->isPointerType()) && qt->getPointeeType()->isRecordType()) {
			if (qt->isPointerType() && qt->getPointeeType()->isRecordType()) {
				isPointer = true; //to properly differentiate among cast types
			}
			if (qt->isReferenceType()) {
				isReference = true;
			}
			const CXXRecordDecl* crd = qt->getPointeeType()->getAsCXXRecordDecl();
			string recordName = crd->getQualifiedNameAsString();

			wrapClassName(recordName);
		}

		return CTypeMetadata(cType, castType, isPointer, isReference, isConst(qt), isVoidType, isFunctionPointer, fpNameLeft, fpNameRight);
	}
};

/**
 * Constructs an equivalent C function given a C++ typefunction
 */
class CFunctionConverter {
	const CXXMethodDecl* const methodDecl;

	string methodName = "";
	string className = "";
	string self = "";
	string separator = "";
	string bodyEnd = "";
	string namespacePrefix;
	string returnType = "";

	std::stringstream functionName;
	std::stringstream functionBody;

	OutputStreams& outputStreamRef;

	string retrieveNamespace() const {
		const DeclContext* context = methodDecl->getParent()->getEnclosingNamespaceContext();
		if (context->isNamespace()) {
			const auto namespaceContext = dyn_cast<NamespaceDecl>(context);
			string namespaceName = namespaceContext->getDeclName().getAsString();
			return (namespaceName.size() != 0 ? (namespaceName + "::") : "");
		}

		return "";
	}

	std::stringstream createFunctionName(const string& returnType, const string& className, const string& methodName) const {
		//TODO: use mangled name?
		std::stringstream functionName;
		functionName << returnType << " " << className << "_" << methodName;

		auto it = funcList.find(functionName.str());
		if(it != funcList.end()) {
			it->second++;
			functionName << "_" << it->second ;
		} else {
			funcList[functionName.str()] = 0;
		}

		functionName << "(";
		if(!methodDecl->isStatic()) {
			functionName << self;
		}

		return functionName;
	}

	void addFunctionCall() {
		//if static call it properly
		if(methodDecl->isStatic()) {
			separator = "";
			functionBody << namespacePrefix + className << "::" << methodName << "(";
		}
		else {
			//use the passed object to call the method
			functionBody << "reinterpret_cast<"<< namespacePrefix + className << "*>(self)->" << methodName << "(";
		}
			
		// Note that the paramaters for the call are added later
		bodyEnd += ")";
	}

	void checkIfValueType(const CTypeMetadata& cTypeMetadata) {
		// before we add the call to the function (and the parameters), 
		// we need to check if we return by value
		if (!cTypeMetadata.isReference() && !cTypeMetadata.isPointer()) {
			// if it is return by value we need to allocate on the heap
			// TODO: we assume here there is a (non-deleted deep-copy) copy constructor 
			functionBody << "new " << namespacePrefix << cTypeMetadata.getCastType() << "(";
			bodyEnd += ")";
		}
	}

	void retrieveAsConstructor() {
		methodName = "create";
		returnType = "W" + className + "*";//TODO only wrap if the class name is in classList
		self = "";
		separator = "";
		functionBody << "return reinterpret_cast<"<< returnType << ">( new " << namespacePrefix + className << "(";
		bodyEnd += "))";
	}

	void retrieveAsDestructor() {
		methodName = "destroy";
		returnType = "void";
		functionBody << "delete reinterpret_cast<"<< namespacePrefix + className << "*>(self)";
	}

	void retrieveAsFunction(CTypeMetadataConverter& typeConverter) {
		methodName = methodDecl->getNameAsString();
		const QualType returnTypeQT = methodDecl->getReturnType();
		CTypeMetadata cReturnTypeMetadata = typeConverter.determineCType(returnTypeQT);
		returnType = cReturnTypeMetadata.getCType();

		if(!cReturnTypeMetadata.isVoidType()) {
			functionBody << "return ";
		}

		// add necessary casts
		if(cReturnTypeMetadata.isReference()) {
			functionBody << "&";
		}

		if(cReturnTypeMetadata.getCastType() != "") {
			functionBody << "reinterpret_cast<"<< returnType << ">(";
			bodyEnd += ")";
		}

		checkIfValueType(cReturnTypeMetadata);

		// if isSmartPtr we need to access the raw pointer so we can pass it to the copy constr to create a copy
		/*int smartPtrIndex = isSmartPtr(cReturnTypeMetadata.getCastType());
		if (smartPtrIndex != -1) {
			functionBody << "*";
		}*/

		addFunctionCall();
	}

public:
	CFunctionConverter(const CXXMethodDecl *c, OutputStreams& o) : methodDecl(c), outputStreamRef(o) {
		className = methodDecl->getParent()->getDeclName().getAsString();
		self = "W" + className + "* self";
		separator = ", ";
	}

	void run (const clang::ASTContext* c, const Resources* cl) {
		CTypeMetadataConverter typeConverter(c, cl);

		namespacePrefix = retrieveNamespace();

		//ignore operator overloadings
		if(methodDecl->isOverloadedOperator()) {
			return;
		}

		// create the function body (note: the parameters are added later)
		if (const CXXConstructorDecl* ccd = dyn_cast<CXXConstructorDecl>(methodDecl)) {
			if(ccd->isCopyConstructor() || ccd->isMoveConstructor()) {
				return;
			}
			retrieveAsConstructor();
		} else if (isa<CXXDestructorDecl>(methodDecl)) {
			retrieveAsDestructor();
		} else {
			retrieveAsFunction(typeConverter);
		}

		functionName = createFunctionName(returnType, className, methodName);
		
		// add the parameters for both the function signature and the function call in the body
		for(unsigned int i = 0; i < methodDecl->getNumParams(); ++i) {
			const ParmVarDecl* param = methodDecl->parameters()[i];
			const QualType parameterQT = param->getType();

			CTypeMetadata cTypeMetadata = typeConverter.determineCType(parameterQT);
			string isConstString = cTypeMetadata.isConst() ? "const " : "" ;

			// in the header, the name is optional, only the type is mandatory
			const string paramName = param->getQualifiedNameAsString().size() != 0 ? param->getQualifiedNameAsString() : ("param" + to_string(i));

			// for the function name, separator needs to be "" initially if constr or static, 
			// otherwise it's ", " because we always have self as a param
			functionName << separator << isConstString << namespacePrefix << cTypeMetadata.getCTypeWithName(paramName) << "";

			// for the function body, add nothing before first param and ", " before the others
			if(i != 0) {
				functionBody << separator;
			}
			
			if(cTypeMetadata.getCastType().size() == 0) {
				functionBody << namespacePrefix << paramName;
			}
			else {
				if(cTypeMetadata.isReference()) {
					// as the parameter is a pointer in C but a reference in C++, dereference it 
					functionBody << "*";
				}
				functionBody << "reinterpret_cast<" << isConstString << namespacePrefix << cTypeMetadata.getCastType() << ">(" << paramName << ")";
			}

			// after dealing with the first parameter, we always add comma
			string separator = ", ";

		}
		functionName << ")";
		functionBody << bodyEnd;
	}

	string getFunctionName() {
		return functionName.str();
	}

	string getFunctionBody() {
		return functionBody.str();
	}
};

/** Handlers **/
class ClassMatchHandler: public MatchFinder::MatchCallback {
public:
	ClassMatchHandler(OutputStreams& os): outputStreamRef(os) {}

	virtual void run(const MatchFinder::MatchResult &Result) {
		if (const CXXMethodDecl *methodDecl = Result.Nodes.getNodeAs<CXXMethodDecl>("publicMethodDecl")) {
			CFunctionConverter functionConverter(methodDecl, outputStreamRef);
			functionConverter.run(Result.Context, classList);

			if (functionConverter.getFunctionName()  == "") {
				// we don't support certain scenarios, so skip those
				return;
			}

			outputStreamRef.headerStream << functionConverter.getFunctionName() << ";\n";

			outputStreamRef.bodyStream << functionConverter.getFunctionName() << "{\n    ";
			outputStreamRef.bodyStream << functionConverter.getFunctionBody() << "; \n}\n" ;
		}
	}

	virtual void onEndOfTranslationUnit() {}

private:
	OutputStreams& outputStreamRef;
};


/****************** /Member Functions *******************************/
// Implementation of the ASTConsumer interface for reading an AST produced
// by the Clang parser. It registers a couple of matchers and runs them on
// the AST.
class MyASTConsumer: public ASTConsumer {
public:
	MyASTConsumer(OutputStreams& os) : outputStreamRef(os),
			classMatcherHandler(os) {
		for(const string& className : classList->getResources()) {
			// TODO add the struct declarations elsewhere as this doesn't work for all cases i.e. template support
			// e.g. foo <int> might be handled by translating to foo_int but at this time we don't know the 
			// template type
			outputStreamRef.headerStream << "struct W" << className << "; \n"
						   "typedef struct W" << className << " W" << className << ";\n";

			// We try to find all public methods of the defined classes
			// TODO: wrap only ANDROID_API classes and methods?
			DeclarationMatcher classMatcher = cxxMethodDecl(isPublic(), ofClass(hasName(className))).bind("publicMethodDecl");
			matcher.addMatcher(classMatcher, &classMatcherHandler);
		}
		outputStreamRef.headerStream << "\n";
	}

	void HandleTranslationUnit(ASTContext &context) override {
		// Run the matchers when we have the whole translation unit parsed.
		matcher.matchAST(context);
	}

private:
	OutputStreams& outputStreamRef;
	ClassMatchHandler classMatcherHandler;
	MatchFinder matcher;
};

// For each source file provided to the tool, a new FrontendAction is created.
// TODO: this means if we specify 2 headers we would generate 2 cwrapper.h, the second overwriting the first
 class MyFrontendAction: public ASTFrontendAction {
public:
	MyFrontendAction() {
		string preprocessorDefineName = getPreprocessorDefineName();

		// Add header guards and necessary includes and typedefs
		outputStream.headerStream << 	"#ifndef _" + preprocessorDefineName + "_CWRAPPER_H_\n"
						"#define _" + preprocessorDefineName + "_CWRAPPER_H_\n"
						"#include \"" + headerForSrcFile + "\"\n"
						"#ifdef __cplusplus\n"
						"typedef bool _Bool;\n"
						"extern \"C\"{\n"
						"#endif\n"
						"#include <stdbool.h>\n";
		outputStream.bodyStream << "#include \"cwrapper.h\"\n"
						"#ifdef __cplusplus\n"
						"extern \"C\"{\n"
						"#endif\n";

	}

	void EndSourceFileAction() override {
		string preprocessorDefineName = getPreprocessorDefineName();

		// Create and write the output files
		StringRef headerFile("cwrapper.h");
		StringRef bodyFile("cwrapper.cpp");

        // Open the output files
        std::error_code error;
        llvm::raw_fd_ostream headerStream(headerFile, error, llvm::sys::fs::F_None);
        if (error) {
            llvm::errs() << "while opening '" << headerFile<< "': "
            << error.message() << '\n';
            exit(1);
        }
        llvm::raw_fd_ostream bodyStream(bodyFile, error, llvm::sys::fs::F_None);
        if (error) {
            llvm::errs() << "while opening '" << bodyFile<< "': "
            << error.message() << '\n';
            exit(1);
        }

		// Header guards end
		outputStream.headerStream << "#ifdef __cplusplus\n"
						"}\n"
						"#endif\n"
						"#endif /* _" + preprocessorDefineName + "_CWRAPPER_H_ */\n";

		outputStream.bodyStream << "#ifdef __cplusplus\n"
						"}\n"
						"#endif\n";

		outputStream.headerStream.flush();
		outputStream.bodyStream.flush();

		// Write to files
		headerStream << outputStream.headerString << "\n";
		bodyStream << outputStream.bodyString << "\n";

	}

	std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance &CI, StringRef file) override {
		// TODO: getCurrentFile here, grab the name and use that instead for generating  the preprocessor name
		return std::make_unique<MyASTConsumer>(outputStream);
	}

private:
	//TODO: consider using std::filesystem::path::filename from c++17
	string getPreprocessorDefineName() {
		string copy = headerForSrcFile;

		auto pos = copy.rfind('/');
		if (pos != string::npos) {
			copy = copy.substr(pos + 1, copy.length());
		}

		pos = copy.rfind('.');
		if (pos != string::npos) {
			copy = copy.substr(0, pos);
		}

		transform(begin(copy), end(copy), begin(copy), ::toupper);

		return copy;
	}

	OutputStreams outputStream;
};

int main(int argc, const char **argv) {
	headerForSrcFile = argv[1]; // the header with the headers we need will be the first parameter

	/** Parse the command-line args passed to your code
	 * Format: cpp2c headers.h -customCmdLineArgs customCmdLineArgsValues -- -IdependentHeader
	 **/
	CommonOptionsParser op(argc, argv, CPP2CCategory);

	string classesToBeWrappedFile = ClassesToBeWrappedArg.getValue();
	classList = new Resources(classesToBeWrappedFile);
	classList->initList();

	// create a new Clang Tool instance (a LibTooling environment)
	ClangTool tool(op.getCompilations(), op.getSourcePathList());

	// run the Clang Tool, creating a new FrontendAction
	return tool.run(newFrontendActionFactory<MyFrontendAction>().get());
}
