#pragma once

//todo: �Ʒ��� SP�� ȣ���ϴ� SQL ��Ʈ���� ����� ����

const wchar_t* SQL_LoadPlayer = L"{ call dbo.spLoadPlayer (?) }"; ///< uid

const wchar_t* SQL_CreatePlayer;//  spCreatePlayer

const wchar_t* SQL_DeletePlayer;// spDeletePlayer

const wchar_t* SQL_UpdatePlayerPosition;// spUpdatePlayerPosition

const wchar_t* SQL_UpdatePlayerComment; //spUpdatePlayerComment

const wchar_t* SQL_UpdatePlayerValid; // spUpdatePlayerValid

