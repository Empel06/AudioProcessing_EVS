# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct C:\devWorks\DTMF\AudioProcessing_EVS\DTMF\AudioProcessing2\platform.tcl
# 
# OR launch xsct and run below command.
# source C:\devWorks\DTMF\AudioProcessing_EVS\DTMF\AudioProcessing2\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {AudioProcessing2}\
-hw {C:\devWorks\SoC_PynzZ2_AUDIO\AudioProcessing2.xsa}\
-out {C:/devWorks/DTMF/AudioProcessing_EVS/DTMF}

platform write
domain create -name {standalone_ps7_cortexa9_0} -display-name {standalone_ps7_cortexa9_0} -os {standalone} -proc {ps7_cortexa9_0} -runtime {cpp} -arch {32-bit} -support-app {hello_world}
platform generate -domains 
platform active {AudioProcessing2}
domain active {zynq_fsbl}
domain active {standalone_ps7_cortexa9_0}
platform generate -quick
platform generate
platform clean
platform generate
