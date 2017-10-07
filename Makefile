BUILDDIR=_build
ROOT=$(PWD)

.PHONY: build
build: $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(ROOT) && $(MAKE)

.PHONY: test
test: $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(ROOT) && $(MAKE) driver test

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)

$(BUILDDIR):
	mkdir $@
