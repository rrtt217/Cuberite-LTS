
// CommandOutput.h

// Declares various classes that process command output

#pragma once





/** Interface for a callback that receives command output
The Out() function is called for any output the command has produced.
Descendants override that function to provide specific processing of the output. */
class cCommandOutputCallback
{
public:
	virtual ~cCommandOutputCallback() {}  // Force a virtual destructor in subclasses

	/** Called when the command wants to output anything; may be called multiple times */
	virtual void Out(const AString & a_Text) = 0;

	/** Outputs the specified text, plus a newline. */
	void OutLn(const AString & aText)
	{
		Out(aText);
		Out("\n");
	}

	/** Called when the command processing has been finished */
	virtual void Finished() {}

	/** Called before Finished() with whether the command was handled (legacy hook).
	Command blocks turn this into their success count (spec section 2.2). Defaults to a no-op.
	Subclasses that know the actual count should override the int overload instead; a subclass
	that overrides only this one still gets called, through the int overload's default. */
	virtual void CommandExecuted(bool a_Success)
	{
		UNUSED(a_Success);
	}

	/** Called before Finished() with the command's success count.
	Vanilla Java 1.12.2 stores the number of affected objects as the command block's SuccessCount
	(spec sections 2.2 and 2.13); a plain success is 1 and a
	failure is 0. The default forwards to the legacy bool overload, so existing subclasses keep
	working unchanged. */
	virtual void CommandExecuted(int a_SuccessCount)
	{
		CommandExecuted(a_SuccessCount > 0);
	}
} ;





/** Class that discards all command output */
class cNullCommandOutputCallback :
	public cCommandOutputCallback
{
	// cCommandOutputCallback overrides:
	virtual void Out(const AString & a_Text) override
	{
		// Do nothing
		UNUSED(a_Text);
	}
} ;





/** Accumulates all command output into a string. */
class cStringAccumCommandOutputCallback:
	public cCommandOutputCallback
{
	using Super = cCommandOutputCallback;

public:

	// cCommandOutputCallback overrides:
	virtual void Out(const AString & a_Text) override;
	virtual void Finished() override {}

	/** Returns the accumulated command output in a string. */
	const AString & GetAccum() const { return m_Accum; }

protected:
	/** Output is stored here until the command finishes processing */
	AString m_Accum;
} ;





/** Sends all command output to a log, line by line, when the command finishes processing */
class cLogCommandOutputCallback :
	public cStringAccumCommandOutputCallback
{
public:
	// cStringAccumCommandOutputCallback overrides:
	virtual void Finished() override;
} ;





/** Sends all command output to a log, line by line; deletes self when command finishes processing */
class cLogCommandDeleteSelfOutputCallback:
	public cLogCommandOutputCallback
{
	using Super = cLogCommandOutputCallback;

	virtual void Finished() override
	{
		Super::Finished();
		delete this;
	}
} ;




