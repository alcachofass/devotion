.PHONY: clean

ifeq ($(QUIET),1)
MAKEFLAGS += -s --no-print-directory
endif

clean:
ifeq ($(QUIET),1)
	@find . -type f -iname '*~' -print0 | xargs -0r rm -f
	@find . -type f -iname '*.swp' -print0 | xargs -0r rm -f
	@find . -type f -iname '*.swo' -print0 | xargs -0r rm -f
else
	find . -type f -iname '*~' -print0 | xargs -0 rm -fv
	find . -type f -iname '*.swp' -print0 | xargs -0 rm -fv
	find . -type f -iname '*.swo' -print0 | xargs -0 rm -fv
endif
