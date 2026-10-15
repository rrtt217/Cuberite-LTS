// CommandLineSplitTest.cpp

// Tests for StringSplitCommandLine(), the command-line splitter that tolerates quoted arguments.
// Spec: specs/commands/vanilla-1.12.2-command-context-and-splitting.md, section 2.1
//
// Invariants verified here:
//   1. For inputs without quotes the result is byte-identical to StringSplit(input, " ") - in particular
//      consecutive / leading spaces still produce empty tokens, because the Core plugin relies on it.
//   2. A single- or double-quoted section anywhere inside a token keeps its spaces together, with the
//      quotes stripped.
//   3. A backslash escapes the next character verbatim (so \" does not open a quoted section).
//   4. An unterminated quote simply extends to the end of the input, without an error.
//
// Why this way: the splitter is a pure function, so a table-driven unit test pins the whole contract
// without needing a running server (see AGENTS.local.md - there is no 1.12.2 oracle on this machine).

#include "Globals.h"
#include "StringUtils.h"
#include "../TestHelpers.h"





/** Renders a vector of strings so that a failing TEST_EQUAL_MSG prints a readable diff. */
static AString VectorToString(const AStringVector & a_Values)
{
	AString res = "[";
	for (size_t i = 0; i < a_Values.size(); i++)
	{
		if (i > 0)
		{
			res += ", ";
		}
		res += "\"" + a_Values[i] + "\"";
	}
	res += "]";
	return res;
}





/** Checks that StringSplitCommandLine() returns exactly a_Expected for a_Input. */
static void CheckSplit(const AString & a_Input, const AStringVector & a_Expected)
{
	const AStringVector Actual = StringSplitCommandLine(a_Input);
	TEST_EQUAL_MSG(VectorToString(Actual), VectorToString(a_Expected), "input: \"" + a_Input + "\"");
}





/** Checks that StringSplitCommandLine() matches StringSplit(input, " ") element by element. */
static void CheckMatchesStringSplit(const AString & a_Input)
{
	const AStringVector Expected = StringSplit(a_Input, " ");
	const AStringVector Actual = StringSplitCommandLine(a_Input);
	TEST_EQUAL_MSG(VectorToString(Actual), VectorToString(Expected), "input (should match StringSplit): \"" + a_Input + "\"");
}





/** Verifies the empty-token behaviour against StringSplit() across the space-only inputs. */
static void testEmptyTokensMatchStringSplit(void)
{
	CheckMatchesStringSplit("");
	CheckMatchesStringSplit(" ");
	CheckMatchesStringSplit("  ");
	CheckMatchesStringSplit("a");
	CheckMatchesStringSplit("a ");
	CheckMatchesStringSplit(" a");
	CheckMatchesStringSplit("a b");
	CheckMatchesStringSplit("a  b");
	CheckMatchesStringSplit("  a  b  ");
	CheckMatchesStringSplit("a b c");
	CheckMatchesStringSplit("/tell  player msg");

	// Explicit expectations, so the test also fails if StringSplit() itself ever changes:
	CheckSplit("", {});
	CheckSplit("a", {"a"});
	CheckSplit("a b", {"a", "b"});
	CheckSplit(" a", {"", "a"});
	CheckSplit("a ", {"a"});
	CheckSplit("a  b", {"a", "", "b"});
	CheckSplit("  ", {"", ""});
}





/** Verifies quoted sections, including quotes in the middle of a token. */
static void testQuotes(void)
{
	CheckSplit("say foo\"bar baz\"", {"say", "foobar baz"});
	CheckSplit("cmd \"foo bar\" baz", {"cmd", "foo bar", "baz"});
	CheckSplit("cmd 'foo bar' baz", {"cmd", "foo bar", "baz"});
	CheckSplit("cmd \"foo bar", {"cmd", "foo bar"});
	CheckSplit("cmd 'foo bar", {"cmd", "foo bar"});
	CheckSplit("cmd \"\"", {"cmd", ""});
	CheckSplit("cmd ''", {"cmd", ""});
	CheckSplit("cmd \"a\"\"b\"", {"cmd", "ab"});
	CheckSplit("cmd \"a 'b' c\"", {"cmd", "a 'b' c"});
	CheckSplit("cmd 'a \"b\" c'", {"cmd", "a \"b\" c"});

	// A quoted section must not swallow the surrounding token:
	CheckSplit("a\"b c\"d", {"ab cd"});
}





/** Verifies backslash escaping. */
static void testEscapes(void)
{
	// \" yields a literal quote and does not open a quoted section, so the space still splits:
	CheckSplit("cmd a\\\"b c", {"cmd", "a\"b", "c"});

	// \\ yields a single backslash:
	CheckSplit("cmd a\\\\b", {"cmd", "a\\b"});

	// A backslash inside quotes escapes there too, and an escaped quote does not close the section:
	CheckSplit("cmd \"a\\\"b c\"", {"cmd", "a\"b c"});

	// A trailing backslash is kept verbatim:
	CheckSplit("cmd a\\", {"cmd", "a\\"});
}





IMPLEMENT_TEST_MAIN("CommandLineSplit",
	testEmptyTokensMatchStringSplit();
	testQuotes();
	testEscapes();
)
