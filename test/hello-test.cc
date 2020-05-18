#include "doctest.h"
#include <cxxopts.hpp>
// has to go first as it violates our requirements

#include "ast/Helpers.h"
#include "ast/ast.h"
#include "ast/treemap/treemap.h"
#include "common/common.h"
#include "core/Error.h"
#include "core/ErrorQueue.h"
#include "core/GlobalSubstitution.h"
#include "core/Unfreeze.h"
#include "core/serialize/serialize.h"
#include "parser/parser.h"
#include "payload/binary/binary.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

using namespace std;

namespace spd = spdlog;

auto logger = spd::stderr_color_mt("hello-test");
auto errorQueue = make_shared<sorbet::core::ErrorQueue>(*logger, *logger);

namespace sorbet {

namespace spd = spdlog;

TEST_CASE("GetGreet") {
    CHECK_EQ("Hello Bazel", "Hello Bazel");
}

TEST_CASE("GetSpdlog") {
    logger->info("Welcome to spdlog!");
}

TEST_CASE("GetCXXopts") {
    cxxopts::Options options("MyProgram", "One line description of MyProgram");
}

TEST_CASE("CountTrees") {
    class Counter {
    public:
        int count = 0;
        unique_ptr<ast::ClassDef> preTransformClassDef(core::MutableContext ctx, unique_ptr<ast::ClassDef> original) {
            count++;
            return original;
        }
        unique_ptr<ast::MethodDef> preTransformMethodDef(core::MutableContext ctx,
                                                         unique_ptr<ast::MethodDef> original) {
            count++;
            return original;
        }

        unique_ptr<ast::If> preTransformIf(core::MutableContext ctx, unique_ptr<ast::If> original) {
            count++;
            return original;
        }

        unique_ptr<ast::While> preTransformWhile(core::MutableContext ctx, unique_ptr<ast::While> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Break> postTransformBreak(core::MutableContext ctx, unique_ptr<ast::Break> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Next> postTransformNext(core::MutableContext ctx, unique_ptr<ast::Next> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Return> preTransformReturn(core::MutableContext ctx, unique_ptr<ast::Return> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Rescue> preTransformRescue(core::MutableContext ctx, unique_ptr<ast::Rescue> original) {
            count++;
            return original;
        }

        unique_ptr<ast::ConstantLit> postTransformConstantLit(core::MutableContext ctx,
                                                              unique_ptr<ast::ConstantLit> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Assign> preTransformAssign(core::MutableContext ctx, unique_ptr<ast::Assign> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Send> preTransformSend(core::MutableContext ctx, unique_ptr<ast::Send> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Hash> preTransformHash(core::MutableContext ctx, unique_ptr<ast::Hash> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Array> preTransformArray(core::MutableContext ctx, unique_ptr<ast::Array> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Literal> postTransformLiteral(core::MutableContext ctx, unique_ptr<ast::Literal> original) {
            count++;
            return original;
        }

        unique_ptr<ast::UnresolvedConstantLit>
        postTransformUnresolvedConstantLit(core::MutableContext ctx, unique_ptr<ast::UnresolvedConstantLit> original) {
            count++;
            return original;
        }

        unique_ptr<ast::Block> preTransformBlock(core::MutableContext ctx, unique_ptr<ast::Block> original) {
            count++;
            return original;
        }

        unique_ptr<ast::InsSeq> preTransformInsSeq(core::MutableContext ctx, unique_ptr<ast::InsSeq> original) {
            count++;
            return original;
        }
    };

    sorbet::core::GlobalState cb(errorQueue);
    cb.initEmpty();
    static constexpr string_view foo_str = "Foo"sv;
    sorbet::core::Loc loc(sorbet::core::FileRef(), 42, 91);
    sorbet::core::UnfreezeNameTable nt(cb);
    sorbet::core::UnfreezeSymbolTable st(cb);

    auto name = cb.enterNameUTF8(foo_str);
    auto classSym = cb.enterClassSymbol(loc, sorbet::core::Symbols::root(), cb.enterNameConstant(name));

    // see if it crashes via failed ENFORCE
    cb.enterTypeMember(loc, classSym, cb.enterNameConstant(name), sorbet::core::Variance::CoVariant);
    auto methodSym = cb.enterMethodSymbol(loc, classSym, name);

    // see if it crashes via failed ENFORCE
    cb.enterTypeArgument(loc, methodSym, cb.enterNameConstant(name), sorbet::core::Variance::CoVariant);

    auto empty = vector<core::SymbolRef>();
    auto argumentSym = core::LocalVariable(name, 0);
    unique_ptr<ast::Expression> rhs(ast::MK::Int(loc.offsets(), 5));
    unique_ptr<ast::Expression> arg = make_unique<ast::Local>(loc.offsets(), argumentSym);
    ast::MethodDef::ARGS_store args;
    args.emplace_back(std::move(arg));

    ast::MethodDef::Flags flags;
    unique_ptr<ast::Expression> methodDef =
        make_unique<ast::MethodDef>(loc.offsets(), loc, methodSym, name, std::move(args), std::move(rhs), flags);
    unique_ptr<ast::Expression> emptyTree = ast::MK::EmptyTree();
    unique_ptr<ast::Expression> cnst =
        make_unique<ast::UnresolvedConstantLit>(loc.offsets(), std::move(emptyTree), name);

    ast::ClassDef::RHS_store classrhs;
    classrhs.emplace_back(std::move(methodDef));
    unique_ptr<ast::Expression> tree =
        make_unique<ast::ClassDef>(loc.offsets(), loc, classSym, std::move(cnst), ast::ClassDef::ANCESTORS_store(),
                                   std::move(classrhs), ast::ClassDef::Kind::Class);
    Counter c;
    sorbet::core::MutableContext ctx(cb, core::Symbols::root(), loc.file());

    auto r = ast::TreeMap::apply(ctx, c, std::move(tree));
    CHECK_EQ(c.count, 3);
}

TEST_CASE("CloneSubstitutePayload") {
    auto logger = spd::stderr_color_mt("ClonePayload");
    auto errorQueue = make_shared<sorbet::core::ErrorQueue>(*logger, *logger);

    sorbet::core::GlobalState gs(errorQueue);
    sorbet::core::serialize::Serializer::loadGlobalState(gs, getNameTablePayload);

    auto c1 = gs.deepCopy();
    auto c2 = gs.deepCopy();

    sorbet::core::NameRef n1;
    {
        sorbet::core::UnfreezeNameTable thaw1(*c1);
        n1 = c1->enterNameUTF8("test new name");
    }

    sorbet::core::GlobalSubstitution subst(*c1, *c2);
    REQUIRE_EQ("<U test new name>", subst.substitute(n1).showRaw(*c2));
    REQUIRE_EQ(c1->allSymbolsUsed(), c2->allSymbolsUsed());
    REQUIRE_EQ(c1->allSymbolsUsed(), gs.allSymbolsUsed());
}
} // namespace sorbet
