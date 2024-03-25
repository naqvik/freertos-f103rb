INC := app/include FreeRTOS-Kernel/include
INC += FreeRTOS-Kernel/portable/GCC/ARM_CM3

disassemble: ./Objects/freertos1.axf
	arm-none-eabi-objdump -dC -M reg-names-std $<

tags :
	etags  app/*.[ch] app/include/*.h FreeRTOS-Kernel/*.c \
           FreeRTOS-Kernel/include/*.h \
           FreeRTOS-Kernel/portable/GCC/ARM_CM3/*.[ch]

mostlyclean :
clean : mostlyclean
	rm -f TAGS
