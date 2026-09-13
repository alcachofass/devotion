.PHONY: clean

ifeq ($(QUIET),1)
MAKEFLAGS += -s --no-print-directory
endif

clean:
ifeq ($(QUIET),1)
	@find . -type f \( -iname '*~' -o -iname '*.swp' -o -iname '*.swo' \) -exec rm -f {} +
else
	find . -type f \( -iname '*~' -o -iname '*.swp' -o -iname '*.swo' \) -exec rm -fv {} +
endif
