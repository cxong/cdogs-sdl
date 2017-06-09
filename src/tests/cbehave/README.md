cbehave - A Behavior Driven Development Framework for C
=======

A demonstration using real C code:

    #include "cbehave.h"

    // Step 1: define your functions
    int add(int a, int b);

    // Step 2: describe behaviour and the function calls
    FEATURE(1, "Addition")
        SCENARIO("Add two numbers")
            GIVEN("we have two numbers 50 and 70")
                int a = 50;
                int b = 70;
            GIVEN_END
            WHEN("we add them together")
                int r = add(a, b);
            WHEN_END
            THEN("the result should be 120")
                SHOULD_INT_EQUAL(r, 120);
            THEN_END
        SCENARIO_END
    FEATURE_END

    // Step 3: write empty implementations of functions
    int add(int a, int b)
    {
        // Step 5: write code to make the behaviour pass
        return a + b;
    }

    // Step 4: run tests and watch them fail (and succeed later)
    int main()
    {
        cbehave_feature calculator_features[] = {
            {feature_idx(1)}
        };

        return cbehave_runner(
            "Calculator Features are as belows:",
            calculator_features);
    }

Recent News
-----------
 - 2013-04-02 fork a copy to https://github.com/bigwhite/cbehave.git.
 - 2012-04-16 cbehave 0.2.0 released.
 - 2011-08-14 cbehave 0.1.0 version released.

Introduction
-------------
CBehave - A Behavior Driven Development Framework for C.

Main Features
-------------

 - use the "feature + scenario" structure (inspired by Cucumber)
 - use classical "given-when-then" template to describe behavior scenarios
 - support mock

A Typical Example
-------------

An example:

    #include <stdio.h>
    #include <errno.h>
    #include "cbehave.h"

    FEATURE(1, "strstr")
      SCENARIO("The strstr finds the first occurrence of the substring in the source string")
 
         GIVEN("A source string: [Lionel Messi is a great football player]")
             char *str = "Lionel Messi is a great football player";
         GIVEN_END
 
         WHEN("we use strstr to find the first occurrence of [football]")
             char *p = strstr(str, "football");
         WHEN_END

         THEN("We should get the string: [football player]")
             SHOULD_STR_EQUAL(p, "football player");
         THEN_END
     SCENARIO_END

     SCENARIO("If strstr could not find the first occurrence of the substring, it will return NULL")

        GIVEN("A source string: FC Barcelona is a great football club.")
            char *str = "FC Barcelona is a great football club";
        GIVEN_END

        WHEN("we use strstr to find the first occurrence of [AC Milan]")
            char *p = strstr(buf, "AC Milan");
        WHEN_END

        THEN("We should get no string but a NULL")
            SHOULD_STR_EQUAL(p, NULL);
        THEN_END
     SCENARIO_END
    FEATURE_END

    int main() {
        cbehave_feature strstr_features[] = {
           {feature_idx(1)},
        };

        return cbehave_runner("Strstr Features are as belows:", strstr_features);
    }
   
The result of the example could be seen like：

   *******************************************************************
       CBEHAVE -- A Behavior Driven Development Framework for C
                By Tony Bai
   *******************************************************************
    Strstr Features are as belows:
    Feature: strstr
     Scenario: The strstr finds the first occurrence of the substring in the source string
         Given: A source string: Lionel Messi is a great football player
         When: we use strstr to find the first occurrence of [football]
         Then: We should get the string: [football player]
     Scenario: If strstr could not find the first occurrence of the substring, it will return NULL
         Given: A source string: FC Barcelona is a great football club.
         When: we use strstr to find the first occurrence of [AC Milan]
         Then: We should get no string but a NULL
     Summary:
     total features: [1]
     failed features: [0]
     total scenarios: [2]
     failed scenarios: [0]

Build
------
 - Download the source code package
 - unzip the package
 - configure->make->make install
 
if you want to compile in 64-bit mode, pass "CPPFLAGS=-m64 LDFLAGS=-m64" to configure.

sometimes, you may encounter such error:

     mv: cannot stat `.deps/cbehave.Tpo': No such file or directory
     make[1]: *** [cbehave.lo] Error 1
	
a solution for this is execute "libtoolize -f" before "configure".


User Guide
-------------
For more information, please open and read the project wiki. cbehave user guide is ready for you.

