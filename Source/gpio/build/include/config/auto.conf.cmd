deps_config := \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/bt/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/esp32/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/ethernet/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/freertos/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/log/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/lwip/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/mbedtls/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/spi_flash/Kconfig \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/bootloader/Kconfig.projbuild \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/esptool_py/Kconfig.projbuild \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/components/partition_table/Kconfig.projbuild \
	/Users/PoloNard1/Documents/GridBallast/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
