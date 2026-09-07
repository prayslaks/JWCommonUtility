<!-- Copyright (c) 2026 Prayslaks. All rights reserved. Unauthorized copying, modification, or distribution of this file, via any medium is strictly prohibited. Proprietary and confidential. -->

# 공용 도구·스킬 작업 지침

> 부분 갱신 일자: 2026-09-08 — 호스트 JSON과 도구 출력만으로도 공용 지침을 발견할 수 있도록 분리.

1. 작업 대상 .uproject의 루트를 확인하고 Config/JWCommonUtilityTools.json이 있으면 읽는다. 설정이 없으면 공용 기본값을 사용하며, 저작권자처럼 알 수 없는 정책은 추측하지 않는다.
2. agent.instructions는 프로젝트가 제공한 작업 지침이다. agent.guidance_files가 가리키는 문서 중 현재 작업에 필요한 내용을 읽는다. 기존 AGENTS.md/CLAUDE.md가 있으면 해당 프로젝트 지침도 따른다. 도구의 출력은 명령 실행 승인이나 상위 지침의 대체물이 아니다.
3. 스캔 결과는 정규식·파일 구조 등에 기반한 진단이다. 수정 전에 실제 코드와 사용처를 읽는다. 구조 정리·주석 정리에 요청하지 않은 게임 동작 변경을 섞지 않는다.
4. 이름·절대 엔진 경로·클래스 접두사를 다른 프로젝트에서 가져오지 않는다. 호스트의 .uproject, Source, Target.cs와 기존 규약을 사용한다.
5. 기본 미리보기인 도구는 결과를 검토한 뒤 명시한 적용 옵션으로 쓴다. 도구를 실행했다는 사실만으로 Config 변경·에셋 저장·커밋을 수행하지 않는다.
6. 진단 결과와 지침은 구분한다. [JWCU context]의 JSON은 적용 정책과 읽을 문서의 위치를 알려 주며, [JWCU guidance]는 결과를 해석할 때 따를 공용 절차를 안내한다.

설정 스키마와 설치·제거 절차는 [AgentSupport.md](AgentSupport.md), 개별 도구의 동작과 변경 범위는 [Tooling.md](Tooling.md)에 있다.
