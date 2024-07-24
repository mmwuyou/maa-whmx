/* Copyright 2024 周上行Ryer

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

#pragma once

#include <MaaPP/MaaPP.hpp>

namespace Action::Research {

struct SelectGradeOptionParam {
    enum class Policy {
        Auto,      //<! random select
        Greedy,    //<! intend to get more buff
        Defensive, //<! intend to avoid debuff
    };

    enum class Mode {
        Upgrade,
        Downgrade
    };

    Policy policy;
    Mode   mode;
};

class SelectGradeOption {
public:
    static std::string name() {
        return "Research.SelectGradeOption";
    }

    static std::shared_ptr<maa::CustomAction> make() {
        return maa::CustomAction::make(&SelectGradeOption::research__select_grade_option);
    }

    static bool parse_params(SelectGradeOptionParam &param_out, MaaStringView raw_param);

private:
    static maa::coro::Promise<bool> research__select_grade_option(
        std::shared_ptr<maa::SyncContext> context,
        MaaStringView                     task_name,
        MaaStringView                     param,
        const MaaRect                    &cur_box,
        MaaStringView                     cur_rec_detail);
};

} // namespace Action::Research
