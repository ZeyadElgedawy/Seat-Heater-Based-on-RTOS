################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
FreeRTOS/Source/portable/CCS/ARM_CM4F/%.obj: ../FreeRTOS/Source/portable/CCS/ARM_CM4F/%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccs1271/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="D:/TivaC_WorkSpace/ADC_test" --include_path="D:/ti/ccs1271/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="D:/TivaC_WorkSpace/ADC_test/Common" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/GPTM" --include_path="D:/TivaC_WorkSpace/ADC_test/FreeRTOS/Source/include" --include_path="D:/TivaC_WorkSpace/ADC_test/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/GPIO" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/UART" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/ADC" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="FreeRTOS/Source/portable/CCS/ARM_CM4F/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS/Source/portable/CCS/ARM_CM4F" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '

FreeRTOS/Source/portable/CCS/ARM_CM4F/%.obj: ../FreeRTOS/Source/portable/CCS/ARM_CM4F/%.asm $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: Arm Compiler'
	"D:/ti/ccs1271/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 -me --include_path="D:/TivaC_WorkSpace/ADC_test" --include_path="D:/ti/ccs1271/ccs/tools/compiler/ti-cgt-arm_20.2.7.LTS/include" --include_path="D:/TivaC_WorkSpace/ADC_test/Common" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/GPTM" --include_path="D:/TivaC_WorkSpace/ADC_test/FreeRTOS/Source/include" --include_path="D:/TivaC_WorkSpace/ADC_test/FreeRTOS/Source/portable/CCS/ARM_CM4F" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/GPIO" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/UART" --include_path="D:/TivaC_WorkSpace/ADC_test/MCAL/ADC" --define=ccs="ccs" --define=PART_TM4C123GH6PM -g --gcc --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="FreeRTOS/Source/portable/CCS/ARM_CM4F/$(basename $(<F)).d_raw" --obj_directory="FreeRTOS/Source/portable/CCS/ARM_CM4F" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: "$<"'
	@echo ' '


