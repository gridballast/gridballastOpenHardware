deps_config := \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/aws_iot/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/bt/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/esp32/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/ethernet/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/fatfs/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/freertos/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/log/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/lwip/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/mbedtls/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/openssl/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/spi_flash/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/PoloNard1/Documents/GridBallast/Support/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
