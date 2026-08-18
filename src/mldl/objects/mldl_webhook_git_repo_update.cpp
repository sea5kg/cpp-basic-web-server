/*
 *
 *         ▜ ▘▗ ▗ ▜      ▌      ▜   ▌
 * ▛▛▌▌▌  ▐ ▌▜▘▜▘▐ █▌  ▛▌█▌▌▌  ▐ ▀▌▛▌
 * ▌▌▌▙▌  ▐▖▌▐▖▐▖▐▖▙▖  ▙▌▙▖▚▘  ▐▖█▌▙▌
 *    ▄▌
 *
 * MIT License
 *
 * Copyright (c) 2025-2026 Evgenii Sopov
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * Original repository: https://github.com/sea5kg/my-little-dev-lab
 */

#include "mldl_webhook_git_repo_update.h"
#include <sea5kg_logger.h>

namespace mldl {

mldl_webhook_git_repo_update::mldl_webhook_git_repo_update(std::shared_ptr<mldl::repository> repo)
    : webhook(repo->webhook_update()), m_repo(repo) {
  TAG = "WEBHOOK_" + repo->webhook_update();
}

bool mldl_webhook_git_repo_update::call() {
  std::string cmd = "git -C " + m_repo->repo_folder() + " pull";
  sea5kg::log::info(TAG, "Try call command '" + cmd + "'");
  if (system(cmd.c_str()) == 0) {
    sea5kg::log::success(TAG, "Command '" + cmd + "' executed.");
    return true;
  }
  sea5kg::log::error(TAG, "Command '" + cmd + "' failed.");
  return false;
}

} // namespace mldl