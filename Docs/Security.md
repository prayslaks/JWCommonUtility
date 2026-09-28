<!-- Copyright (c) 2026 Prayslaks. SPDX-License-Identifier: MIT -->

# OS 암호화와 개인 API 키 저장

> 부분 갱신 일자: 2026-09-28 — 공용 AI 공급자 키 관리 스킬을 추가하고 호스트 전사 통합 절차와 분리.

공급자 등록·키 관리 연결 작업은 [unreal-ai-provider-keys](../Agent/Skills/unreal-ai-provider-keys/SKILL.md)를 따른다. 이 문서는 저장·선택·이관 계약의 정본이며 전사기나 게임 어댑터 통합은 호스트가 소유한다.

> 부분 갱신 일자: 2026-09-28 — 고정 공급자 카탈로그에 ElevenLabs 추가. 기존 암호화 목록·키 선택 UI 재사용.

> 부분 갱신 일자: 2026-09-28 — 공급자 목록을 코드로 고정하고 Provider Registry 편집을 제거, 설정·GIS 검증 기준을 통일.

> 부분 갱신 일자: 2026-09-28 — AI Provider Settings·관리 화면을 JWCU로 이관하고 GameInstance별 BP 서브시스템을 추가.

> 부분 갱신 일자: 2026-09-27 — 공급자별 다중 API 키 관리 화면·사용 키 선택·관리용 만료일과 암호화 목록 이관을 반영.

> 부분 갱신 일자: 2026-09-27 — Windows DPAPI와 공급자 독립 API 키 저장소 추가.

## 범위

`UJWCU_BFL_PlatformCrypto`는 현재 Windows 사용자 계정에 결합된 DPAPI 데이터 보호를 제공한다. 별도 암호키를 코드나 설정에 넣지 않는다. `CRYPTPROTECT_UI_FORBIDDEN`을 사용하고 컴퓨터 전체 계정에 권한을 주는 `CRYPTPROTECT_LOCAL_MACHINE`은 사용하지 않는다. Windows 외 플랫폼에서는 암호화·복호화가 false를 반환하며 평문 대체 저장은 없다.

`UJWCU_BFL_APIKeyStore`는 네트워크·AI 공급자·GameInstance에 의존하지 않는 게임 스레드 전용 저장소다. 호스트가 안정적인 AppId와 KeyName을 지정한다. 각 식별자는 ASCII 영숫자·하이픈·밑줄 1~128자로 제한하며 개인 정보나 키 자체를 식별자로 쓰지 않는다.

## C++ / Blueprint 사용

| 함수 | 동작 |
| --- | --- |
| ProtectData / UnprotectData | 바이트 배열과 동일한 Context로 암호화·복호화. 실패 시 출력은 비움. 입출력에 같은 배열을 전달하지 않음 |
| SetApiKey(AppId, KeyName, Key, false) | 실행 중 덮어쓰기. 디스크 변경 없음. 빈 값은 저장 키 사용을 임시 차단 |
| SetApiKey(AppId, KeyName, Key, true) | 공백 제거 후 암호화 저장. 빈 값이면 Forget. 실패 시 false |
| GetApiKey | 실행 중 덮어쓰기 우선, 없으면 암호문을 복호화해 문자열 반환. 미존재·손상·다른 계정이면 false와 빈 출력 |
| HasApiKey | 비어 있지 않은 키를 읽을 수 있는지 확인 |
| HasStoredApiKey | 파일 존재 여부만 확인. 손상 파일도 true이므로 복구 시 덮어쓰기 방지에 사용 |
| GetStoredApiKey (C++ 전용) | 실행 중 덮어쓰기를 무시하고 저장본 조회 |
| ClearMemoryKey | 실행 중 덮어쓰기 버퍼를 지우고 저장본 사용으로 복귀 |
| ForgetApiKey | 저장본 삭제 성공 후 메모리도 제거. 삭제 실패는 false |

Blueprint의 **JWCU → Security → API Keys**에서 호출한다. GetApiKey의 출력은 민감한 값이므로 Print String·로그·SaveGame·에셋 기본값에 연결하지 않는다. 공급자가 아닌 일반 문자열 키 이름을 쓰므로 다른 서비스의 개인 API 키도 저장할 수 있다. 저장 문자열은 최대 131,072 TCHAR이며 내부 NUL은 거절한다.

## 프로젝트 설정과 Blueprint

설정 위치는 **Project Settings → JWCommonUtility → AI Provider Settings**다. `UJWCU_AIProviderSettings`와 `JWCommonUtilityEditor`의 Details 커스터마이징이 공급자 카드·키 추가/수정 대화상자·사용 키 선택·삭제·목록 새로고침을 소유한다. 비밀값은 Config 속성이 아니다. 공급자 목록은 UJWCU_AIProviderSettings::GetProviderCatalog()의 OpenAI/Gemini/ElevenLabs로 고정한다. 사용자는 공급자 카드를 제거하거나 ID를 편집하지 않고 키만 관리한다. 공급자 추가·지원 종료는 플러그인 업데이트로 처리하며, bDeprecated=true인 공급자는 카드와 저장 키 조회·삭제만 유지하고 새 키 등록·선택·런타임 사용은 거절한다. 기존 Config의 ProviderIds는 더 이상 읽지 않는다. [Deprecated 2026-09-28] 편집 가능한 Provider Registry.

BP에서는 **Get Game Instance Subsystem → JWCU AI Provider Subsystem**을 얻는다. 공급자는 enum 대신 고정 목록의 문자열 ID를 받는다. GetSupportedProviders로 현재 지원 ID를 얻으며 미등록 ID는 설정·GIS 양쪽에서 거절한다. 저수준 APIKeyStore/Vault는 범용 저장소이므로 임의의 유효 식별자를 계속 지원한다.

- 저장: SaveKey(Provider, 빈 Id, Label, ApiKey, ExpiresUtc) → 성공 시 OutId. 첫 키 자동 선택, 이후 SelectKey(Provider, Id).
- 조회: ListKeys는 비밀 없는 목록, GetApiKey는 실제 사용 키, HasApiKey는 읽기 가능 여부를 반환한다.
- 편집: 기존 Id로 SaveKey를 호출하며 ApiKey가 빈 값이면 비밀값을 유지한다.
- 삭제: DeleteKey는 로컬 항목 하나, DeleteAllKeys는 해당 공급자의 전체 저장 목록을 삭제한다. 공급자 서버에는 요청하지 않는다.
- 임시 적용: SetRuntimeApiKey·ClearRuntimeApiKey·HasRuntimeOverride는 해당 GameInstance의 메모리만 다룬다. 빈 임시 키는 저장 키 조회를 가린다. GIS Deinitialize/BeginDestroy에서 임시 문자열을 지운다.

영구 저장 네임스페이스는 `FApp::GetProjectName()`이며 프로젝트 이름이 바뀌면 별도 저장소가 된다. 키 목록과 사용 키 선택은 같은 프로젝트의 GameInstance들이 공유한다. GIS에서 선택·사용 키 편집·사용 키 삭제를 수행하면 호출 인스턴스의 임시 덮어쓰기를 해제한다. 다른 인스턴스의 임시 키와 에디터 선택 변경은 서로 간섭하지 않는다. 에디터처럼 GameInstance가 없는 네이티브 호출자는 설정의 GetApiKey를 사용할 수 있다.

플러그인은 ZK 타입이나 ZK ini 섹션을 참조하지 않는다. ProjectZK에는 이전 평문 ini 이관과 Deprecated 정적 BP 호환만 남긴다. 이 호환 경로의 옛 실행 중 덮어쓰기는 프로세스 전역이므로 신규 BP는 GIS를 사용한다. 저수준 암호화·Vault BFL은 다른 도구나 네이티브 호스트를 위한 공용 기능으로 유지한다.

## 다중 키 목록

`UJWCU_BFL_APIKeyVault`는 AppId·Group별 키 목록과 사용 키 ID를 하나의 암호화 JSON에 저장한다. Group은 ASCII 영숫자·하이픈·밑줄 1~122자이며 APIKeyStore의 `Vault-<Group>` 이름을 예약한다. 직접 이 이름에 다른 데이터를 쓰지 않는다. 키와 이름·등록/수정/관리용 만료일 모두 같은 암호문으로 원자 교체한다. 최대 32개, 키 16,384 TCHAR, 이름 64자, 직렬화된 전체 목록 131,072 TCHAR 제한을 넘으면 기존 저장본을 보존하고 실패한다.

| 함수 | 계약 |
| --- | --- |
| ListKeys | 비밀 원문 없이 FJWCU_APIKeyInfo 목록 반환. 끝 4자리만 표시하며 8자 이하 키는 전부 마스킹 |
| SaveKey | 빈 Id면 새 항목, 기존 Id면 수정. 수정 시 빈 ApiKey는 기존 원문 유지. 첫 항목만 자동 선택 |
| SelectKey | 해당 키 선택 후 원래 Group의 런타임 덮어쓰기를 해제 |
| GetActiveKey | 원래 Group의 런타임 덮어쓰기(빈 값 차단 포함) 우선, 아니면 선택 키 반환 |
| DeleteKey | 해당 로컬 항목 삭제. 사용 키 삭제 시 선택을 비우며 다른 키를 자동 선택하지 않음 |
| DeleteAllKeys | 빈 목록을 저장한 뒤 이전 단일 키와 실행 중 덮어쓰기를 제거 |
| MigrateSingleKey | 기존 단일 키를 목록으로 검증 저장한 뒤 제거. 기존 목록(빈 목록 포함)이 우선. 이관 키의 원래 생성/수정 시각은 미지정 |

ExpiresUtc는 사용자 지정 관리용 날짜다. 실제 공급자의 만료·권한·할당량을 조회하거나 API 사용을 차단하지 않는다. 삭제도 로컬 저장본에만 적용하며 원격 키 폐기는 공급자 사이트에서 수행한다. 등록·선택·수정 실패는 false이며 손상 목록은 추가/수정으로 덮어쓰지 않는다. 선택 변경은 다음 조회부터 적용되며 이미 시작된 요청의 인증은 바꾸지 않는다.

## 저장과 수명

저장 위치는 `FPlatformProcess::UserSettingsDir()/JWCommonUtility/APIKeys/<식별자 해시>.bin`이다. Windows에서는 사용자 로컬 AppData 아래이며 프로젝트·Git 작업 폴더 밖이다. 경로 해시는 식별자 구분용이고 키의 암호화는 DPAPI가 담당한다. AppId/KeyName으로 만든 버전 포함 Context가 추가 엔트로피이므로 다른 이름의 파일을 바꿔 끼우면 복호화되지 않는다.

저장은 같은 폴더의 고유 임시 파일에 암호문을 쓴 뒤 다시 읽어 복호화 결과를 원문과 비교한다. 성공한 파일만 `MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH)`로 교체하며 실패 시 기존 저장본을 보존한다. 임시 파일도 암호문이고 완료·실패 후 삭제한다. 일반 UE Move는 목적 파일을 먼저 삭제할 수 있어 사용하지 않는다.

실행 중 덮어쓰기는 모듈 메모리에만 있으며 모듈 종료 때 지운다. 복호화는 호출 시 수행하므로 재시작해도 저장본을 읽을 수 있다. 처리 중 바이트 버퍼와 관리 중인 문자열은 사용 후 덮어쓰지만, 호출자·변환기·엔진이 만든 모든 문자열 복사본이나 메모리 덤프까지 제거한다고 보장하지 않는다. 같은 사용자 권한으로 실행되는 악성 프로세스에 대한 격리는 제공하지 않는다. 계정·컴퓨터 변경 등으로 복원이 안 되면 키를 다시 입력해야 할 수 있다.

## 호스트 이관 규칙

기존 평문 설정 이관은 호스트가 소유한다. 암호문이 이미 있으면 먼저 복호화 가능 여부를 확인하고 기존 저장값을 우선한다. 없으면 SetApiKey(..., true)가 성공한 후에만 평문 설정을 삭제한다. 손상 암호문·읽기 실패 때 자동으로 평문으로 대체하거나 기존 키를 덮어쓰지 않는다. 평문 잔여분 삭제가 실패하면 호출자에게 실패를 알리고 재시도한다.

## 검증

자동 테스트 `JWCommonUtility.Security.PlatformCrypto`와 `JWCommonUtility.Security.APIKeyStore`는 가짜 데이터와 고유 AppId를 사용한다. 실제 Windows DPAPI의 이진 왕복·Context 불일치·변조 거절, 메모리 우선순위·UTF8 저장·평문 미포함·읽기 전용 파일 교체 실패 시 원본 유지·정상 교체·손상 파일 거절·삭제를 검사한다. 실제 API·마이크는 사용하지 않는다. 다른 Windows 계정에서의 거절은 동일 계정 자동 테스트 범위 밖이다.

2026-09-27 검증: 호스트 ProjectZKEditor Win64 Development 빌드 성공. JWCU 보안 2종과 호스트 음성 회귀 6종, 총 8개 테스트 통과. 저장 파일 교체 실패와 UE 5.7 Config 캐시 식별자 해석을 포함한다. 실제 호스트 로컬 키도 값 출력 없이 암호문 생성·평문 ini 제거를 확인했다.

2026-09-28 검증: JWCU 보안 4종(암호화·단일 저장·다중 목록·GIS 격리), 에디터 날짜 입력 1종, 호스트 음성 6종, 총 11개 성공·실패 0. GIS별 임시 키 격리·종료 정리·공유 저장 선택과 레거시 이관을 검증했다. 보고서: 호스트 `Saved/Automation/JWCUAIProviders/index.json`. 실제 키 값이나 외부 API는 테스트에 사용하지 않았다. 화면의 육안 검수는 별도다.

2026-09-28 추가 검증: 고정 공급자 목록·편집 속성 제거·미등록 ID 거절을 포함하여 Editor/Game 빌드와 11개 회귀 테스트 통과. 호스트 보고서: `Saved/Automation/JWCUAIProvidersFixedCatalog/index.json`.

## 구현 위치

- [PlatformCrypto 헤더](../Source/JWCommonUtility/Public/JWCU_BFL_PlatformCrypto.h), [구현](../Source/JWCommonUtility/Private/JWCU_BFL_PlatformCrypto.cpp)
- [APIKeyStore 헤더](../Source/JWCommonUtility/Public/JWCU_BFL_APIKeyStore.h), [구현](../Source/JWCommonUtility/Private/JWCU_BFL_APIKeyStore.cpp)
- [AIProviderSettings](../Source/JWCommonUtility/Public/JWCU_AIProviderSettings.h), [AIProviderSubsystem](../Source/JWCommonUtility/Public/JWCU_AIProviderSubsystem.h)
- [APIKeyVault](../Source/JWCommonUtility/Public/JWCU_BFL_APIKeyVault.h), [Editor 관리 화면](../Source/JWCommonUtilityEditor/Private/JWCU_AIProviderKeysCustomization.cpp)
- [자동 테스트](../Source/JWCommonUtility/Private/JWCU_SecurityTests.cpp)
- [Microsoft CryptProtectData](https://learn.microsoft.com/en-us/windows/win32/api/dpapi/nf-dpapi-cryptprotectdata)
