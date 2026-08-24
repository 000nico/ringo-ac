typedef struct FLAGGED_DLL {\
	WCHAR NameBuffer[256];
	UNICODE_STRING FullImageName;

	BOOLEAN isSigned;

	struct {
		struct {
			PVOID ImageBase;
			SIZE_T ImageSize;
		} Image;
		
	} Mem;
};

VOID SetLoadImageCallback();