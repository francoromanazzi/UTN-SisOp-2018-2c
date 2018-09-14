regordts: init_deps install_shared make_projects

init_deps:
	@echo "Fetchig dependecies from github"
	@git submodule update --init --recursive
	@echo "Installing SO-COMMONS-LIBRARY"
	@cd commons && sudo $(MAKE) install

install_shared:
	@echo "Installing shared library"
	@cd shared && sudo $(MAKE) install

make_projects:
	@echo "Building all projects"
	@cd CPU && $(MAKE)
	@cd DAM && $(MAKE)
	@cd FM9 && $(MAKE)
	@cd MDJ && $(MAKE)
	@cd S-AFA && $(MAKE)
	@echo "Finished!"

.PHONY: regordts init_deps install_shared make_projects