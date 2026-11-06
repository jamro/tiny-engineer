# Tasks

## 1. Policy: npm skip + memo

- [x] 1.1 In `policy.js`, memoize `sourceHash` by package root and export a clear function for tests; verify two calls for the same root return the same digest and a clear + file edit yields a new digest
- [x] 1.2 In `loadIdentity`, for channel `npm` set a stable empty/sentinel `sourceHash` without calling the content walk; verify npm identity uses that sentinel and existing `decideDaemonAction` npm cases still pass

## 2. Suite

- [x] 2.1 Run `npm test --prefix packages/tiny-engineer-cli -- test/daemon-policy.test.js` (and full package suite if anything else imports identity); verify exit 0
