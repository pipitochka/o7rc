#include <fstream>
#include <iostream>
#include <memory>
#include <string>

#include <parser/IParser.h>
#include <tokenizer/ITokenizer.h>
#include <util/ast/Ast.h>
#include <sema/Sema.h>

#ifdef USE_FLEX
#include <tokenizer/impl/flex/FlexTokenizer.h>
#endif

#ifdef USE_HAND_TOKENIZER
#include <tokenizer/impl/hand/HandTokenizer.h>
#endif

#ifdef USE_BISON
#include <parser/impl/bison/BisonParser.h>
#endif

#ifdef USE_HAND_PARSER
#include <parser/impl/hand/HandParser.h>
#endif

#ifdef USE_DEBUG
#include <tokenizer/impl/debug/BufferedTokenizer.h>
#endif

#ifdef USE_CODEGEN
#include <codegen/RiscVCodeGen.h>
#endif

#include <ir/IRBuilder.h>
#include <ir/IRPrinter.h>
#include <ir/IRDotExporter.h>
#include <ir/PassManager.h>
#include <ir/RiscVIRCodeGen.h>
#include <ir/passes/ConstantFolding.h>
#include <ir/passes/CopyPropagation.h>
#include <ir/passes/DeadCodeElim.h>
#include <module/ModuleLoader.h>

int main(int argc, char* argv[]) {
    std::ios::sync_with_stdio(false);

    std::string inputPath;
    std::string outputPath;
    std::string tokenizerChoice;
    std::string parserChoice;
    bool useIR = false;
    bool dumpIR = false;
    bool dumpIRPasses = false;
    bool optimize = false;
    bool skipSema = false;
    std::string dotPath;
    std::vector<std::string> modulePaths;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "-o" && i + 1 < argc) {
            outputPath = argv[++i];
        } else if (arg == "--tokenizer" && i + 1 < argc) {
            tokenizerChoice = argv[++i];
        } else if (arg == "--parser" && i + 1 < argc) {
            parserChoice = argv[++i];
        } else if (arg == "--ir") {
            useIR = true;
        } else if (arg == "--dump-ir") {
            useIR = true;
            dumpIR = true;
        } else if (arg == "--dump-ir-passes") {
            useIR = true;
            dumpIRPasses = true;
            optimize = true;
        } else if (arg == "--opt") {
            useIR = true;
            optimize = true;
        } else if (arg == "--dump-dot" && i + 1 < argc) {
            useIR = true;
            dotPath = argv[++i];
        } else if (arg == "--no-sema") {
            skipSema = true;
        } else if ((arg == "--module-path" || arg == "-M") && i + 1 < argc) {
            modulePaths.push_back(argv[++i]);
        } else if (inputPath.empty()) {
            inputPath = arg;
        }
    }

    if (inputPath.empty()) {
        std::cout << "Enter path to source file: ";
        if (!std::getline(std::cin, inputPath) || inputPath.empty()) {
            std::cerr << "No path provided\n";
            return 1;
        }
    }

    if (outputPath.empty()) {
        std::cout << "Enter path to output file: ";
        if (!std::getline(std::cin, outputPath) || outputPath.empty()) {
            std::cerr << "No path provided\n";
            return 1;
        }
    }

    auto file = std::make_unique<std::ifstream>(inputPath);
    if (!*file) {
        std::cerr << "Cannot open file: " << inputPath << "\n";
        return 1;
    }

    ITokenizerPtr tokenizer;

#if defined(USE_FLEX) && defined(USE_HAND_TOKENIZER)
    if (tokenizerChoice == "hand")
        tokenizer = std::make_unique<HandTokenizer>(*file);
    else
        tokenizer = std::make_unique<FlexTokenizer>(*file);
#elif defined(USE_FLEX)
    tokenizer = std::make_unique<FlexTokenizer>(*file);
#elif defined(USE_HAND_TOKENIZER)
    tokenizer = std::make_unique<HandTokenizer>(*file);
#endif

#ifdef USE_DEBUG
    auto buffered = std::make_unique<BufferedTokenizer>(std::move(tokenizer));

    try {
        buffered->check();
    } catch (const std::exception &ex) {
        std::cerr << ex.what() << "\n";
        return 1;
    }

    buffered->print();

    tokenizer = std::move(buffered);
#endif

    std::unique_ptr<IParser> parser;

#if defined(USE_BISON) && defined(USE_HAND_PARSER)
    if (parserChoice == "hand")
        parser = std::make_unique<HandParser>();
    else
        parser = std::make_unique<BisonParser>();
#elif defined(USE_BISON)
    parser = std::make_unique<BisonParser>();
#elif defined(USE_HAND_PARSER)
    parser = std::make_unique<HandParser>();
#endif

    if (!tokenizer) {
        std::cerr << "No tokenizer built/selected\n";
        return 1;
    }

    if (!parser) {
        std::cerr << "No parser built/selected\n";
        return 1;
    }

    auto result = parser->parse(tokenizer);

    if (!result) {
        std::cerr << "Parsing failed\n";
        return 1;
    }

    std::cout << "Parsed module: " << result->name << "\n";

    // --- Module loading ---
    auto makeParserForPath = [&](const std::string& path) -> ModulePtr {
        auto modFile = std::make_unique<std::ifstream>(path);
        if (!modFile || !*modFile) return nullptr;
        ITokenizerPtr modTok;
#if defined(USE_FLEX) && defined(USE_HAND_TOKENIZER)
        if (tokenizerChoice == "hand")
            modTok = std::make_shared<HandTokenizer>(*modFile);
        else
            modTok = std::make_shared<FlexTokenizer>(*modFile);
#elif defined(USE_FLEX)
        modTok = std::make_shared<FlexTokenizer>(*modFile);
#elif defined(USE_HAND_TOKENIZER)
        modTok = std::make_shared<HandTokenizer>(*modFile);
#endif
        auto modParser = std::unique_ptr<IParser>();
#if defined(USE_BISON) && defined(USE_HAND_PARSER)
        if (parserChoice == "hand")
            modParser = std::make_unique<HandParser>();
        else
            modParser = std::make_unique<BisonParser>();
#elif defined(USE_BISON)
        modParser = std::make_unique<BisonParser>();
#elif defined(USE_HAND_PARSER)
        modParser = std::make_unique<HandParser>();
#endif
        if (!modTok || !modParser) return nullptr;
        return modParser->parse(modTok);
    };

    ModuleLoader moduleLoader;
    moduleLoader.setParserFactory(makeParserForPath);

    {
        std::string inputDir = inputPath;
        auto slashPos = inputDir.rfind('/');
        if (slashPos != std::string::npos)
            inputDir = inputDir.substr(0, slashPos);
        else
            inputDir = ".";
        moduleLoader.addSearchPath(inputDir);
    }

    for (auto& mp : modulePaths)
        moduleLoader.addSearchPath(mp);

    if (!moduleLoader.loadImports(*result)) {
        std::cerr << "Failed to load imported modules\n";
        return 1;
    }

    if (!skipSema) {
        Sema sema;
        for (auto& mi : moduleLoader.loaded()) {
            auto errors = sema.analyze(*mi.ast);
            if (!errors.empty()) {
                for (auto& e : errors)
                    std::cerr << "sema [" << mi.name << "]: " << e.str() << "\n";
                return 1;
            }
        }

        auto semaErrors = sema.analyze(*result);
        if (!semaErrors.empty()) {
            for (auto& e : semaErrors)
                std::cerr << "sema: " << e.str() << "\n";
            return 1;
        }
    }

    if (outputPath.empty()) {
        outputPath = inputPath;
        auto dotPos = outputPath.rfind('.');
        if (dotPos != std::string::npos)
            outputPath = outputPath.substr(0, dotPos);
        outputPath += ".asm";
    }

    const auto& loadedModules = moduleLoader.loaded();

    if (useIR) {
        IRBuilder irBuilder;
        auto irMod = loadedModules.empty()
            ? irBuilder.build(*result)
            : irBuilder.build(*result, loadedModules);

        if (dumpIR) {
            IRPrinter printer;
            std::cerr << "\n";
            printer.print(irMod, std::cerr);
            std::cerr << "\n";
        }

        if (optimize) {
            PassManager pm;
            pm.add<ConstantFolding>()
              .add<CopyPropagation>()
              .add<DeadCodeElim>();

            std::ostream* log = dumpIRPasses ? &std::cerr : nullptr;
            pm.run(irMod, log);

            if (dumpIR) {
                std::cerr << "; === After optimization ===\n\n";
                IRPrinter printer;
                printer.print(irMod, std::cerr);
                std::cerr << "\n";
            }
        }

        if (!dotPath.empty()) {
            std::ofstream dotFile(dotPath);
            if (dotFile) {
                IRDotExporter exporter;
                exporter.exportModule(irMod, dotFile);
                dotFile.close();
                std::cout << "CFG exported: " << dotPath << "\n";
            }
        }

        std::ofstream outFile(outputPath);
        if (!outFile) {
            std::cerr << "Cannot open output file: " << outputPath << "\n";
            return 1;
        }

        RiscVIRCodeGen irCodegen;
        irCodegen.generate(irMod, outFile);
        outFile.close();
        std::cout << "Generated (IR): " << outputPath << "\n";
    }
#ifdef USE_CODEGEN
    else {
        std::ofstream outFile(outputPath);
        if (!outFile) {
            std::cerr << "Cannot open output file: " << outputPath << "\n";
            return 1;
        }

        RiscVCodeGen codegen;
        if (loadedModules.empty())
            codegen.generate(*result, outFile);
        else
            codegen.generate(*result, outFile, loadedModules);
        outFile.close();
        std::cout << "Generated: " << outputPath << "\n";
    }
#endif

    return 0;
}
