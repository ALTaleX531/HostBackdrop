#pragma once
// Windows ͷ?ļ?
#include <windows.h>
#include <Unknwn.h>
#include <roapi.h>
#include <DispatcherQueue.h>
// Windows Runtime Library
#include <wrl.h>
#include <wrl\client.h>
#include <wrl\module.h>
#include <wrl\implements.h>
#include <wrl\wrappers\corewrappers.h>

#pragma comment(lib, "windowsapp")

namespace
{
	using namespace Microsoft::WRL;
	using namespace Microsoft::WRL::Wrappers;

	inline void HR(const HRESULT& hr)
	{
		if (FAILED(hr))
		{
			throw hr;
		}
	}

	template <class T>
	inline auto GetActivationFactory(HSTRING activatableClassId)
	{
		ComPtr<T> pFactory{nullptr};

		HRESULT hr = RoGetActivationFactory(activatableClassId, IID_PPV_ARGS(&pFactory));
		if (hr == REGDB_E_CLASSNOTREG)
		{
			CO_MTA_USAGE_COOKIE pCookie{nullptr};
			HR(CoIncrementMTAUsage(&pCookie));
			hr = RoGetActivationFactory(activatableClassId, IID_PPV_ARGS(&pFactory));
		}
		HR(hr);

		return pFactory;
	}

	template <typename T>
	inline void CreateInstanceWithFactory(HSTRING activatableClassId, IInspectable** pInspectable)
	{
		ComPtr<IInspectable> baseInterface;
		ComPtr<IInspectable> innerInterface;
		ComPtr<T> pFactory{GetActivationFactory<T>(activatableClassId)};
		HR(pFactory->CreateInstance(baseInterface.Get(), &innerInterface, pInspectable));
	}
	inline void ActivateInstanceWithFactory(HSTRING activatableClassId, IInspectable** pInspectable)
	{
		ComPtr<IActivationFactory> pFactory{GetActivationFactory<IActivationFactory>(activatableClassId)};
		HR(pFactory->ActivateInstance(pInspectable));
	}

	template <typename T>
	inline auto RoCast(IInspectable* pInspectable)
	{
		ComPtr<T> instance{nullptr};
		HR(pInspectable->QueryInterface(IID_PPV_ARGS(&instance)));
		return instance;
	}
	template <typename T, typename Ptr>
	inline auto RoCast(const ComPtr<Ptr>& pInspectable)
	{
		ComPtr<T> instance{nullptr};
		HR(pInspectable.As(&instance));
		return instance;
	}
}