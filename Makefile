BUILDDIR=_build
ROOT=$(PWD)

.PHONY: build
build: $(BUILDDIR)
	cd $(BUILDDIR) && cmake $(ROOT) && $(MAKE)

.PHONY: debug
debug: $(BUILDDIR)
	cd $(BUILDDIR) && cmake -DCMAKE_BUILD_TYPE=Debug $(ROOT) && $(MAKE)

.PHONY: test
test:
	./run-tests.sh

.PHONY: clean
clean:
	rm -rf $(BUILDDIR)


.PHONY: compose
compose:
	docker-compose build
	docker-compose run --rm test

.PHONY: up
up:
	docker-compose -f docker-compose.local.yml up -d

.PHONY: down
down:
	docker-compose -f docker-compose.local.yml down

$(BUILDDIR):
	mkdir $@
