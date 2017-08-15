# Build dt-overlays for Hilscher netX4000 based platforms

$(obj)/%.dtbo: $(src)/%.dtso FORCE
	$(call if_changed_dep,dtc)

dtso := $(shell find $(srctree)/arch/arm/boot/dts/netx4000/overlays -name *.dtso -exec basename {} \;)
dtbo-y += ${dtso:.dtso=.dtbo}

always 		:= $(dtbo-y)
clean-files	:= *.dtbo
