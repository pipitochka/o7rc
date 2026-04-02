#pragma once
#include "Function.h"
#include "passes/IPass.h"
#include <memory>
#include <ostream>
#include <vector>

class PassManager {
public:
    /// Добавляет проход по типу: pm.add<ConstantFolding>()
    template<typename T, typename... Args>
    PassManager& add(Args&&... args) {
        passes_.push_back(std::make_unique<T>(std::forward<Args>(args)...));
        return *this;
    }

    /// Добавляет проход по указателю
    PassManager& addPass(std::unique_ptr<IPass> pass);

    /// Запускает все проходы на модуле. Если log != nullptr, печатает IR после каждого прохода.
    void run(IRModule& mod, std::ostream* log = nullptr);

    /// Количество зарегистрированных проходов
    size_t size() const { return passes_.size(); }

    /// Очищает список проходов
    void clear() { passes_.clear(); }

private:
    std::vector<std::unique_ptr<IPass>> passes_;
    void runOnFunction(IRFunction& fn, std::ostream* log);
};
