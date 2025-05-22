compiler_cxx := g++
settings_compiler_cxx := -Og

huinit: huinit.o
	"${compiler_cxx}" "${^}" '-o' "${@}"

huinit.o: huinit.cxx
	"${compiler_cxx}" ${settings_compiler_cxx} '-c' "${^}"
