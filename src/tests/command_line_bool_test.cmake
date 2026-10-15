# Exercise boolean options through the installed game's command-line parser.
file(MAKE_DIRECTORY "${TEST_HOME}")
set(ENV{HOME} "${TEST_HOME}")
set(ENV{XDG_CONFIG_HOME} "${TEST_HOME}")
set(ENV{APPDATA} "${TEST_HOME}")
foreach(value true false)
  execute_process(
    COMMAND "${GAME}" "--config=Game.FriendlyFire,${value}" --config
    RESULT_VARIABLE result
    OUTPUT_VARIABLE output
    ERROR_VARIABLE error)
  if(NOT result EQUAL 0 OR NOT output MATCHES "FriendlyFire"
      OR output MATCHES "Video Options:")
    message(FATAL_ERROR "Boolean option ${value} did not continue parsing: ${output}${error}")
  endif()
endforeach()
