ПУСТО умножение_матриц (ВЕЩ m1[][], ВЕЩ m2[][], ВЕЩ rez[][])
{
    ЦЕЛ i, j, k;
    ЦЕЛ m = кол_во(m1), kk = кол_во(m1[0]), n = кол_во(m2[0]);
    // реальные границы m1, m2 и rez получат от фактических параметров
    // стандартная функция кол_во (upb) выдает число элементов по
    // заданному измерению (первый параметр) массива (второй параметр)
    ВЕЩ сум;
    ДЛЯ (i  = 0; i < m; i++)
        ДЛЯ (j = 0; j < n; j++)
        {
            сум = 0;
            ДЛЯ (k = 0; k < kk; k++)
                сум += m1[i][k] * m2[k][j];
            rez[i][j] = сум;
        }
}


ПУСТО главная()
{
    ЦЕЛ mm, kk, nn;
    читатьид(mm, kk, nn);
    {
        // были операторы, поэтому для описания матриц нужно
        // открыть новый блок
//        ВЕЩ мат1[mm][] = {{1,2,3}, {4,5,6}}, мат2[kk][] = {{0,3}, {1,4}, {2,5}}, рез[mm][nn];
        ВЕЩ мат1[mm][kk], мат2[kk][nn], рез[mm][nn];
        читатьид(мат1, мат2);
        умножение_матриц(мат1, мат2, рез);
        печатьид(рез);
    }
}
