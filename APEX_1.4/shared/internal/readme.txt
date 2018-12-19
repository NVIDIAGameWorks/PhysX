The intent of "shared" is to provide utility code that samples and tools
can share.

*** Shared code MUST go in "internal" if it is ALSO used by
*** the APEX SDK or by any APEX Module.

*** However, code in "internal" MUST be self-contained. ***

That is, it should not rely on the APEX SDK.  Otherwise tools need to
drag in all of APEX in order to compile, and we end up with a statically
linked APEX library living along side a dynamically linked APEX library.

Description (from the APEX Architecture Document):

    *  Internal shared code (in APEX/shared/internal) - this is
    used by APEX internal code, but is designed to be self-contained.
    That is, it doesn't rely on code from the framework or modules.
    In this way, "friend" projects like tools can share this code
    with APEX without having to include the the rest of the APEX
    source. For example, streaming utilities are defined here, as
    well as the APEX streaming version number. Tools need this to
    create and stream out APEX objects using the proper versioning
    system.

    * Public shared code (in APEX/shared/external) - these are
    utilities that tools and external applications like sample apps
    can share. For example, an implementation of the user renderer
    is here, used by the DestructionTool and SimpleDestruction
    sample. Code in this category should not be used by internal
    APEX source. This helps to decouple the code used for applications
    from the APEX SDK version.

