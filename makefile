regordts: init_deps install_shared make_projects

init_deps:
	@echo "Fetchig dependecies from github"
	@git submodule init && git submodule update
	@echo "Installing SO-COMMONS-LIBRARY"
	@cd commons && sudo $(MAKE) install

install_shared:
	@echo "Installing shared library"
	@cd shared && sudo $(MAKE) install

make_projects:
	@echo "Building all projects"
	@cd CPU/Debug && $(MAKE)
	@cd DAM/Debug && $(MAKE)
	@cd FM9/Debug && $(MAKE)
	@cd MDJ/Debug && $(MAKE)
	@cd S-AFA/Debug && $(MAKE)
	@echo "Finished!"

.PHONY: regordts init_deps install_shared make_projects