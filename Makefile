.PHONY: huinit

huinit: huinit.mk
	make --makefile="${<}"
