subdirs = thirdParty \
	common \
	0_dependenciesTest \
	1_helloWorld \
	2_classicModel \
	3_clippingPlanes \
	4_mipmapVis \
	5_bumpMapping \
	6_cubeMaps

.PHONY: $(subdirs)

all: $(subdirs)

$(subdirs):
	$(MAKE) -C $@
	@echo ---
	@echo ---

