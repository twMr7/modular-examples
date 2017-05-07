AppLoadPlugin
-------------

本範例修改自 [Poco](http://pocoproject.org/) 的 "SampleApp" 專案。 範例中展示了封裝在 **Application** 類別的 *config*，*logger*，和命令列 *defineOptions* 方法，以及如何優雅地用來建立功能豐富的應用程式。 針對軟體的模組化，這個專案也展示了如何用 **ClassLoader** ，根據不同的設定，從外部 plugin 動態載入、建立對應的類別實體。

展示重點：

1. 要載入的 plugin 需繼承自應用程式專案中已知的抽象父代類別，本範例中為 **AbstractPlugin** 及 **AbstractOtherPlugin**。
2. 注意 PluginC 中，如何用 **POCO_BEGIN_MANIFEST** 的 macro 將所需的類別宣言輸出。
3. 注意 PluginAB 中，PluginA 及 PluginB 分別繼承自不同父代，如何分別將不同的類別宣言輸出。
4. **loadPlugin()** 方法中，主要是透過 **Poco::ClassLoader.loadLibrary()** 載入 .dll 檔，Iterator 檢查是否存在需要的類別，然後 **Poco::ClassLoader.create()** 動態產生類別的實體。
