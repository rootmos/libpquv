BUILDDIR=_build
ROOT=$(PWD)

.PHONY: build
build: $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(ROOT) && $(MAKE)

.PHONY: test
test:
	./run-tests.sh

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)


.PHONY: compose
compose:
	docker-compose build
	docker-compose run test

$(BUILDDIR):
	mkdir $@
