#ifndef __SMC35X_H
#define __SMC35X_H

enum pl353_smc_ecc_mode {
	PL353_SMC_ECCMODE_BYPASS = 0,
	PL353_SMC_ECCMODE_APB = 1,
	PL353_SMC_ECCMODE_MEM = 2
};

enum pl353_smc_mem_width {
	PL353_SMC_MEM_WIDTH_8 = 0,
	PL353_SMC_MEM_WIDTH_16 = 1
};

void smc35x_set_buswidth(struct device *dev, uint8_t cs_addr, unsigned int bw);
int  smc35x_get_nand_int_status_raw(struct device *dev, int ifc);
void smc35x_clr_nand_int(struct device *dev, int ifc);
int  smc35x_set_ecc_mode(struct device *dev, enum pl353_smc_ecc_mode mode);
int  smc35x_ecc_is_busy(struct device *dev);
u32  smc35x_get_ecc_val(struct device *dev, int ecc_reg);
int  smc35x_set_ecc_pg_size(struct device *dev, unsigned int pg_sz);

#endif				/* __SMC35X_H */
