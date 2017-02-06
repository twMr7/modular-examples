AppLoadPlugin
-------------

本範例修改自 [Poco](http://pocoproject.org/) 的 "SampleApp" 專案。 範例中展示了封裝在 **Application** 類別的 *config*，*logger*，和命令列 *defineOptions* 方法，以及如何優雅地用來建立功能豐富的應用程式。 針對軟體的模組化，這個專案也展示了如何用 **ClassLoader** ，根據不同的設定，從外部 plugin 動態載入、建立對應的類別實體。

展示重點：

1. 在 **wmain()** 裡控制繼承自 Poco::Application 的主要應用程式流程。
2. 如何透過 **Application::defineOptions()** 方法處理命令參數選項。
3. 在 **Application::initialize()** 如何載入預設或指定的設定檔。
4. 設定檔中包含針對 **logger** 的不同設定，請試著修改專案中的兩個ini設定檔內容，以及試著指定程式載入不同設定檔，試試參數改變的差異。
5. 注意 **logger** 在程式中如何輸出不同等級的日誌？
6. 叫用 **logger().trace()** 方法 和 **poco_trace()** 巨集有何差異？
5. 要載入的 plugin 需繼承自應用程式專案中已知的抽象父代類別，本範例中為 **AbstractPlugin** 及 **AbstractOtherPlugin**。
6. 注意 PluginC 中，如何用 **POCO_BEGIN_MANIFEST** 的 macro 將所需的類別宣言輸出。
7. 注意 PluginAB 中，PluginA 及 PluginB 分別繼承自不同父代，如何分別將不同的類別宣言輸出。
8. **loadPlugin()** 方法中，主要是透過 **Poco::ClassLoader.loadLibrary()** 載入 .dll 檔，Iterator 檢查是否存在需要的類別，然後 **Poco::ClassLoader.create()** 動態產生類別的實體。
