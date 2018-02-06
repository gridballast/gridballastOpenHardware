deps_config := \
	/home/ro269/esp/esp-idf/components/app_trace/Kconfig \
	/home/ro269/esp/esp-idf/components/aws_iot/Kconfig \
	/home/ro269/esp/esp-idf/components/bt/Kconfig \
	/home/ro269/esp/esp-idf/components/esp32/Kconfig \
	/home/ro269/esp/esp-idf/components/ethernet/Kconfig \
	/home/ro269/esp/esp-idf/components/fatfs/Kconfig \
	/home/ro269/esp/esp-idf/components/freertos/Kconfig \
	/home/ro269/esp/esp-idf/components/heap/Kconfig \
	/home/ro269/esp/esp-idf/components/libsodium/Kconfig \
	/home/ro269/esp/esp-idf/components/log/Kconfig \
	/home/ro269/esp/esp-idf/components/lwip/Kconfig \
	/home/ro269/esp/esp-idf/components/mbedtls/Kconfig \
	/home/ro269/esp/esp-idf/components/openssl/Kconfig \
	/home/ro269/esp/esp-idf/components/pthread/Kconfig \
	/home/ro269/esp/esp-idf/components/spi_flash/Kconfig \
	/home/ro269/esp/esp-idf/components/spiffs/Kconfig \
	/home/ro269/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/ro269/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/ro269/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/ro269/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/ro269/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/ro269/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)


$(deps_config): ;
