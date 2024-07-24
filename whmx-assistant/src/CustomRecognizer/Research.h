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

namespace Rec::Research {

class ParseGradeOptionsOnModify {
public:
    static std::string name() {
        return "Research.ParseGradeOptionsOnModify";
    }

    static std::shared_ptr<maa::CustomRecognizer> make() {
        return maa::CustomRecognizer::make(&ParseGradeOptionsOnModify::research__parse_grade_options_on_modify);
    }

private:
    static maa::coro::Promise<maa::AnalyzeResult> research__parse_grade_options_on_modify(
        maa::SyncContextHandle context, maa::ImageHandle image, std::string_view task_name, std::string_view param);
};

} // namespace Rec::Research
