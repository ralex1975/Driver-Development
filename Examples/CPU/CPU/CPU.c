/* Very Simple Driver Example
 * Marcus Botacin - 2018
 * Federal University of Paran� (UFPR)
 */

#include<fltKernel.h>
#include<intrin.h>

#define DRIVERNAME L"\\Device\\ReadWrite"
#define DOSDRIVERNAME L"\\DosDevices\\ReadWrite"
#define MAX_MSG_STRING 1024

VOID UnloadRoutine(PDRIVER_OBJECT  DriverObject)
{
	UNICODE_STRING path;
	UNREFERENCED_PARAMETER(DriverObject);
	DbgPrint("Bye...");
	RtlInitUnicodeString(&path,DOSDRIVERNAME);
	IoDeleteSymbolicLink(&path);
	IoDeleteDevice(DriverObject->DeviceObject);
}

char data[MAX_MSG_STRING];

#define CPUID_MAX_STRING 12
#define CPUID_NUM_REGS 4
#define CPUID_VENDOR_INFO 0
#define CPUID_PROC_FEATURES 1

#define REG_EAX 0
#define REG_EBX 1
#define REG_ECX 2
#define REG_EDX 3

VOID get_cpu_info()
{
	int regs[CPUID_NUM_REGS];
	__cpuid(regs,CPUID_VENDOR_INFO);
	memcpy(&data[0],&regs[REG_EBX],sizeof(int));
	memcpy(&data[4],&regs[REG_EDX],sizeof(int));
	memcpy(&data[8],&regs[REG_ECX],sizeof(int));
	data[CPUID_MAX_STRING]='\0';
}

NTSTATUS Write(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	PVOID userbuffer;
	PIO_STACK_LOCATION PIO_STACK_IRP;
	UINT32 datasize,sizerequired;
	NTSTATUS NtStatus = STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(DeviceObject);

	/* if data is available, copy to userland */
		userbuffer=Irp->AssociatedIrp.SystemBuffer;
		PIO_STACK_IRP=IoGetCurrentIrpStackLocation(Irp);
		if(PIO_STACK_IRP && userbuffer)
		{
			datasize=PIO_STACK_IRP->Parameters.Write.Length;
			sizerequired=sizeof(char)*MAX_MSG_STRING;
			if(datasize<sizerequired){
				memcpy(data,userbuffer,datasize);
				DbgPrint(data);
				Irp->IoStatus.Status = NtStatus;
				Irp->IoStatus.Information = sizerequired;
			}else{
				Irp->IoStatus.Status = NtStatus;
				Irp->IoStatus.Information = 0;
			}
		}

	Irp->IoStatus.Status=STATUS_SUCCESS;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);

	return NtStatus;

}

/* Write data from driver to the userland stack */
NTSTATUS Read(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	PVOID userbuffer;
	PIO_STACK_LOCATION PIO_STACK_IRP;
	UINT32 datasize,sizerequired;
	NTSTATUS NtStatus=STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(DeviceObject);
	NtStatus=STATUS_SUCCESS;

		/* if data is available, copy to userland */
		userbuffer=Irp->AssociatedIrp.SystemBuffer;
		PIO_STACK_IRP=IoGetCurrentIrpStackLocation(Irp);
		if(PIO_STACK_IRP && userbuffer)
		{
			get_cpu_info();
			datasize=PIO_STACK_IRP->Parameters.Read.Length;
			sizerequired=sizeof(char)*strlen(data)+1;
			if(datasize>=sizerequired){
				memcpy(userbuffer,data,sizerequired);
				Irp->IoStatus.Status = NtStatus;
				Irp->IoStatus.Information = sizerequired;
			}else{
				Irp->IoStatus.Status = NtStatus;
				Irp->IoStatus.Information = 0;
			}
		}else{
			Irp->IoStatus.Status = NtStatus;
			Irp->IoStatus.Information = 0;
		}
	

	IoCompleteRequest(Irp, IO_NO_INCREMENT);

	return NtStatus;

}

/* Create File -- start capture mechanism */
NTSTATUS Create(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	int i;
	NTSTATUS status;
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("Create I/O operation");
	Irp->IoStatus.Status=STATUS_SUCCESS;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	status = STATUS_SUCCESS;
	return status;
}


/* CLoseFile/CloseHandle -- stop routine */
NTSTATUS Close(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	int i;
	NTSTATUS status;
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("Close");
	Irp->IoStatus.Status=STATUS_SUCCESS;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	status = STATUS_SUCCESS;
	return status;
}

NTSTATUS NotSupported(PDEVICE_OBJECT DeviceObject,PIRP Irp)
{
	NTSTATUS status;
	UNREFERENCED_PARAMETER(DeviceObject);
	DbgPrint("Not Supported I/O operation");
	Irp->IoStatus.Status=STATUS_NOT_SUPPORTED;
	IoCompleteRequest(Irp,IO_NO_INCREMENT);
	status = STATUS_NOT_SUPPORTED;
	return status;
}

NTSTATUS
DriverEntry (
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )

{
	int i;
	PDEVICE_OBJECT dev;
	UNICODE_STRING namestring,linkstring;
    NTSTATUS status=STATUS_SUCCESS;
	UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER( RegistryPath );
	DbgPrint("Hello from the Kernel");
	DriverObject->DriverUnload=UnloadRoutine;
	
	RtlInitUnicodeString(&namestring,DRIVERNAME); 
	status=IoCreateDevice(DriverObject,0,&namestring,FILE_DEVICE_DISK_FILE_SYSTEM,FILE_DEVICE_SECURE_OPEN,FALSE,&dev);
	if(!NT_SUCCESS(status))
	{
		DbgPrint("Error Creating Device");
		return status;
	}

	DriverObject->DeviceObject=dev;

	RtlInitUnicodeString(&linkstring,DOSDRIVERNAME);
	status=IoCreateSymbolicLink(&linkstring,&namestring); /* Linking the name */
	if(!NT_SUCCESS(status))
	{
		DbgPrint("Error Creating Link");
		IoDeleteDevice(dev);
		return status;
	}


	/* registering generic I/O routines */
	for(i=0;i<IRP_MJ_MAXIMUM_FUNCTION;i++)
	{
		DriverObject->MajorFunction[i]=NotSupported;
	}

	DriverObject->MajorFunction[IRP_MJ_CREATE]=Create;
	DriverObject->MajorFunction[IRP_MJ_WRITE]=Write;
	DriverObject->MajorFunction[IRP_MJ_READ]=Read;
	DriverObject->MajorFunction[IRP_MJ_CLOSE]=Close;

	dev->Flags|=DO_BUFFERED_IO;

    return status;
}