#pragma once

namespace WML {
class Core final {
   public:
    static Core& GetInstance();

   private:
    Core() = default;
    ~Core() = default;

    Core(const Core&) = delete;
    Core& operator=(const Core&) = delete;
    Core(Core&&) = delete;
    Core& operator=(Core&&) = delete;
};
}  // namespace WML
