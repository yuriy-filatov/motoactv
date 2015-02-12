LBL_OUT=$(PRODUCT_OUT)/obj/PARTITIONS/lbl_intermediates
LBL=$(LBL_OUT)/lbl

.PHONY: $(LBL)
$(LBL):
	make -f motorola/external/lbl/Makefile all \
			LBL_OUT=$(LBL_OUT) HOST_PREBUILT_TAG=$(HOST_PREBUILT_TAG)

$(PRODUCT_OUT)/lbl: $(LBL)
	cp -f $< $@
