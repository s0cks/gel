// #include <gmock/gmock.h>
// #include <gtest/gtest.h>

// #include <fstream>

// #include "common.h"
// #include "expr/expression.h"
// #include "expr/expression_dot.h"
// #include "module.h"
// #include "parser.h"
// #include "gmock/gmock.h"

// namespace gel {
// using ::testing::AssertionResult, ::testing::AssertionFailure, ::testing::AssertionSuccess, testing::Test;
// class ParserTest : public Test {};

// #define DEFINE_EXPRESSION_MATCHER(Name) \
//   MATCHER(Is##Name, "") {               \
//     return arg && arg->Is##Name();      \
//   }
// FOR_EACH_EXPRESSION_NODE(DEFINE_EXPRESSION_MATCHER)  // NOLINT
// #undef DEFINE_EXPRESSION_MATCHER

// MATCHER(IsConstTrue, "") {  // NOLINT
//   return arg && (arg->IsBool() && arg->AsBool()->Get() == true);
// }

// MATCHER(IsConstFalse, "") {  // NOLINT
//   return arg && (arg->IsBool() && arg->AsBool()->Get() == false);
// }

// MATCHER_P(IsConstLong, rhs, "") {  // NOLINT
//   return arg && (arg->IsLong() && arg->AsLong()->Get() == rhs);
// }

// MATCHER_P(IsConstDouble, rhs, "") {  // NOLINT
//   return arg && arg->IsDouble() && (arg->AsDouble()->Get() == rhs);
// }

// MATCHER_P(IsConstString, rhs, "") {  // NOLINT
//   return arg && arg->IsString() && arg->AsString()->Get() == rhs;
// }

// MATCHER(IsLiteralTrue, "") {  // NOLINT
//   return testing::ExplainMatchResult(IsLiteralExpr(), arg, result_listener) &&
//          testing::ExplainMatchResult(IsConstTrue(), arg->AsLiteralExpr()->GetValue(), result_listener);
// }

// MATCHER(IsLiteralFalse, "") {  // NOLINT
//   return testing::ExplainMatchResult(IsLiteralExpr(), arg, result_listener) &&
//          testing::ExplainMatchResult(IsConstFalse(), arg->AsLiteralExpr()->GetValue(), result_listener);
// }

// MATCHER_P(IsLiteralLong, rhs, "") {  // NOLINT
//   return testing::ExplainMatchResult(IsLiteralExpr(), arg, result_listener) &&
//          testing::ExplainMatchResult(IsConstLong(rhs), arg->AsLiteralExpr()->GetValue(), result_listener);
// }

// MATCHER_P(IsLiteralDouble, rhs, "") {  // NOLINT
//   return testing::ExplainMatchResult(IsLiteralExpr(), arg, result_listener) &&
//          testing::ExplainMatchResult(IsConstDouble(rhs), arg->AsLiteralExpr()->GetValue(), result_listener);
// }

// MATCHER_P(IsLiteralString, rhs, "") {  // NOLINT
//   return testing::ExplainMatchResult(IsLiteralExpr(), arg, result_listener) &&
//          testing::ExplainMatchResult(IsConstString(rhs), arg->AsLiteralExpr()->GetValue(), result_listener);
// }

// TEST_F(ParserTest, Test_Parse_Literal_True_Lowercase) {  // NOLINT
//   const auto expr = Parser::ParseExpr("#t");
//   ASSERT_THAT(expr, IsLiteralTrue());
// }

// TEST_F(ParserTest, Test_Parse_Literal_True_Uppercase) {  // NOLINT
//   const auto expr = Parser::ParseExpr("#T");
//   ASSERT_THAT(expr->GetBody(), IsLiteralTrue());
// }

// TEST_F(ParserTest, Test_Parse_Literal_False_Lowercase) {  // NOLINT
//   const auto expr = Parser::ParseExpr("#f");
//   ASSERT_THAT(expr, IsLiteralFalse());
// }

// TEST_F(ParserTest, Test_Parse_Literal_False_Uppercase) {  // NOLINT
//   const auto expr = Parser::ParseExpr("#F");
//   ASSERT_THAT(expr, IsLiteralFalse());
// }

// TEST_F(ParserTest, Test_Parse_Literal_Long) {  // NOLINT
//   const auto expr = Parser::ParseExpr("1287902");
//   ASSERT_THAT(expr, IsLiteralLong(1287902));
// }

// TEST_F(ParserTest, Test_Parse_Literal_Double) {  // NOLINT
//   const auto expr = Parser::ParseExpr("3.141592654");
//   ASSERT_THAT(expr, IsLiteralDouble(3.141592654));
// }

// TEST_F(ParserTest, Test_Parse_Literal_String) {  // NOLINT
//   const auto expr = Parser::ParseExpr("\"Hello World\"");
//   ASSERT_THAT(expr, IsLiteralString("Hello World"));
// }

// // TEST_F(ParserTest, Test_Parse_ListExpr_0) {
// //   const auto expr = Parser::ParseExpr("()");
// //   ASSERT_THAT(expr, ::testing::AllOf(::testing::NotNull(), IsListExpr()));
// //   ASSERT_EQ(expr->GetNumberOfChildren(), 0);
// // }

// // TEST_F(ParserTest, Test_Parse_ListExpr_1) {
// //   const auto expr = Parser::ParseExpr("(1)");
// //   ASSERT_THAT(expr, ::testing::AllOf(::testing::NotNull(), IsListExpr()));
// //   ASSERT_EQ(expr->GetNumberOfChildren(), 1);
// //   ASSERT_THAT(expr->GetChildAt(0), ::testing::AllOf(::testing::NotNull(), IsLiteralLong(1)));
// // }

// // TEST_F(ParserTest, Test_Parse_ListExpr_2) {
// //   const auto expr = Parser::ParseExpr("(1 2)");
// //   ASSERT_THAT(expr, ::testing::AllOf(::testing::NotNull(), IsListExpr()));
// //   ASSERT_EQ(expr->GetNumberOfChildren(), 2);
// //   ASSERT_THAT(expr->GetChildAt(0), ::testing::AllOf(::testing::NotNull(), IsLiteralLong(1)));
// //   ASSERT_THAT(expr->GetChildAt(1), ::testing::AllOf(::testing::NotNull(), IsLiteralLong(2)));
// // }

// // TEST_F(ParserTest, Test_Parse_ListExpr_3) {
// //   const auto expr = Parser::ParseExpr("(1 2 3)");
// //   ASSERT_THAT(expr, ::testing::AllOf(::testing::NotNull(), IsListExpr()));
// //   ASSERT_EQ(expr->GetNumberOfChildren(), 3);
// //   ASSERT_THAT(expr->GetChildAt(0), ::testing::AllOf(::testing::NotNull(), IsLiteralLong(1)));
// //   ASSERT_THAT(expr->GetChildAt(1), ::testing::AllOf(::testing::NotNull(), IsLiteralLong(2)));
// //   ASSERT_THAT(expr->GetChildAt(2), ::testing::AllOf(::testing::NotNull(), IsLiteralLong(3)));
// // }

// TEST_F(ParserTest, Test_Parse_LiteralLambda_EmptyNoNameArgsAndDocs) {
//   const auto expr = Parser::ParseExpr("(fn [])");
//   DLOG(INFO) << "expr: " << expr;
// }

// TEST_F(ParserTest, Test_Parse_LiteralLambda_EmptyWithNameAndNoArgsOrDocs) {
//   const auto expr = Parser::ParseExpr("(fn test [])");
//   DLOG(INFO) << "expr: " << expr;
// }

// TEST_F(ParserTest, Test_Parse_LiteralLambda_EmptyWithNameArgsAndNoDocs) {
//   const auto expr = Parser::ParseExpr("(fn test [arg0])");
//   DLOG(INFO) << "expr: " << expr;
// }

// TEST_F(ParserTest, Test_Parse_LiteralLambda_WithNameArgsAndDocs) {
//   const auto expr = Parser::ParseExpr("(fn test [arg0] \"This is a test function\")");
//   DLOG(INFO) << "expr: " << expr;
// }

// TEST_F(ParserTest, Test_Parse_Kernel) {  // NOLINT
//   const auto home = GetHomeEnvVar().value();
//   ASSERT_TRUE(home);
//   ASSERT_THAT((*home), ::testing::Not(::testing::IsEmpty()));
//   const auto kernel_path = fmt::format("{}/_kernel.cl", (*home));
//   const auto kernel = Parser::ParseModuleFrom(kernel_path);
//   ASSERT_THAT(kernel, ::testing::NotNull());
// }
// }  // namespace gel
