# Raven tests - an application driven from outside through the environment
# (RAVEN_HEADLESS, RAVEN_SCRIPT, RAVEN_TREE, RAVEN_SNAP), as a user, a CI job
# or an agent would run it. The application is headless_app.c: it knows
# nothing of the tests and returns 0 from main.
#
#   cmake -DAPP=path/to/headless_app -DCASE=tree|script|fail|syntax|quit|snap -DOUT=folder -P headless_cli.cmake
#
# A case that cannot run here (snap without a GPU) prints SKIPPED, which
# ctest reports as skipped (SKIP_REGULAR_EXPRESSION).

if(NOT APP OR NOT CASE OR NOT OUT)
	message(FATAL_ERROR "usage: cmake -DAPP=... -DCASE=... -DOUT=... -P headless_cli.cmake")
endif()
file(REMOVE_RECURSE "${OUT}")
file(MAKE_DIRECTORY "${OUT}")

# Runs the application with environment variables; sets rc, out, err.
function(run_app)
	execute_process(COMMAND "${CMAKE_COMMAND}" -E env ${ARGN} "${APP}"
		WORKING_DIRECTORY "${OUT}"
		RESULT_VARIABLE result OUTPUT_VARIABLE output ERROR_VARIABLE errors TIMEOUT 120)
	message(STATUS "exit code ${result}\n--- stdout\n${output}--- stderr\n${errors}---")
	set(rc "${result}" PARENT_SCOPE)
	set(out "${output}" PARENT_SCOPE)
	set(err "${errors}" PARENT_SCOPE)
endfunction()

function(expect_contains text part what)
	string(FIND "${text}" "${part}" at)
	if(at EQUAL -1)
		message(FATAL_ERROR "${what} does not contain: ${part}")
	endif()
endfunction()

function(expect_rc expected)
	if(NOT "${rc}" STREQUAL "${expected}")
		message(FATAL_ERROR "exit code ${rc}, expected ${expected}")
	endif()
endfunction()

if(CASE STREQUAL "tree")
	# the tree of a window, without a script: what an agent reads first
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_TREE=tree.json)
	expect_rc(0)
	file(READ "${OUT}/tree.json" json)
	expect_contains("${json}" "\"title\": \"Sign in\"" tree.json)
	expect_contains("${json}" "\"size\": [340, 260]" tree.json)
	expect_contains("${json}" "\"path\": \"form/user\"" tree.json)
	expect_contains("${json}" "\"role\": \"text field\"" tree.json)
	expect_contains("${json}" "\"text\": \"Sign in to Raven\"" tree.json)
	expect_contains("${json}" "\"checked\": false" tree.json)
	expect_contains("${json}" "\"bounds\": [20, 20, 300," tree.json)
	# also to the standard output
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_TREE=-)
	expect_rc(0)
	expect_contains("${out}" "\"path\": \"form/submit\"" stdout)

elseif(CASE STREQUAL "script")
	# clicks and typing change the state; the script checks it, the tree shows it
	file(WRITE "${OUT}/signin.script" [=[
# a user signs in
expect status text ""
click form/submit
expect status text "Type a user name"
click user
type "ada"
click "text:Remember me"
expect remember checked
click form/submit
expect status text "Welcome, ada"
mark signed-in
tree after.json
]=])
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_SCRIPT=signin.script RAVEN_TREE=final.json)
	expect_rc(0)
	expect_contains("${err}" "signin.script passed" stderr)
	expect_contains("${err}" "mark signed-in" stderr)
	foreach(name after.json final.json)
		file(READ "${OUT}/${name}" json)
		expect_contains("${json}" "\"text\": \"Welcome, ada\"" ${name})
		expect_contains("${json}" "\"checked\": true" ${name})
		expect_contains("${json}" "\"text\": \"ada\"" ${name})
	endforeach()

elseif(CASE STREQUAL "fail")
	# a check that is false fails the process (main returns 0), with the line and what was read
	file(WRITE "${OUT}/fail.script" [=[
click form/submit
expect status text "Welcome, nobody"
]=])
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_SCRIPT=fail.script RAVEN_TIMEOUT=0.2 RAVEN_TREE=failed.json)
	expect_rc(1)
	expect_contains("${err}" "FAILED fail.script:2: expect" stderr)
	expect_contains("${err}" "reads \"Type a user name\"" stderr)
	# the tree is still written: what the interface was when it failed
	file(READ "${OUT}/failed.json" json)
	expect_contains("${json}" "\"text\": \"Type a user name\"" failed.json)

elseif(CASE STREQUAL "syntax")
	# a mistake in the script stops the run before it starts (exit code 2)
	file(WRITE "${OUT}/typo.script" "click form/submit\nclik form/submit\n")
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_SCRIPT=typo.script)
	expect_rc(2)
	expect_contains("${err}" "line 2: unknown command \"clik\"" stderr)
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_SCRIPT=missing.script)
	expect_rc(2)
	expect_contains("${err}" "cannot read the script" stderr)

elseif(CASE STREQUAL "quit")
	# the script ends the application through its own button: a success
	file(WRITE "${OUT}/quit.script" "click form/quit\n")
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_SCRIPT=quit.script)
	expect_rc(0)
	# a step after the end of the application is a failure
	file(WRITE "${OUT}/after.script" "click form/quit\nclick form/submit\n")
	run_app(RAVEN_HEADLESS=1 RAVEN_BACKEND=null RAVEN_SCRIPT=after.script)
	expect_rc(1)
	expect_contains("${err}" "after.script:2: click: the application ended before this step" stderr)

elseif(CASE STREQUAL "snap")
	# pictures with the GPU backend of the platform, when there is one
	file(WRITE "${OUT}/snap.script" "type user \"ada\"\nsnap typed\nsnap shots/button form/submit\n")
	run_app(RAVEN_HEADLESS=1 RAVEN_SCRIPT=snap.script RAVEN_SNAP=final.png RAVEN_SCALE=2)
	string(FIND "${err}" "null backend" fallback)
	if(NOT fallback EQUAL -1)
		message(STATUS "SKIPPED: no GPU backend here (${err})")
		return()
	endif()
	expect_rc(0)
	foreach(name final.png typed.png shots/button.png)
		if(NOT EXISTS "${OUT}/${name}")
			message(FATAL_ERROR "${name} was not written")
		endif()
		file(READ "${OUT}/${name}" signature LIMIT 8 HEX)
		if(NOT signature STREQUAL "89504e470d0a1a0a")
			message(FATAL_ERROR "${name} is not a PNG file")
		endif()
	endforeach()
	# RAVEN_SCALE=2: the window of 340 x 260 is captured with 680 x 520 pixels (the width is bytes 16..19 of the PNG)
	file(READ "${OUT}/final.png" header LIMIT 24 HEX)
	string(SUBSTRING "${header}" 32 16 size)
	if(NOT size STREQUAL "000002a800000208")
		message(FATAL_ERROR "final.png is not 680 x 520 pixels (header ${size})")
	endif()

else()
	message(FATAL_ERROR "unknown case ${CASE}")
endif()
