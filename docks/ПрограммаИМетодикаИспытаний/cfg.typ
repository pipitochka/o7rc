#let cfg(data) = (
    project: (
        code: [RU.17701729.05.#data.code 12 51-1],
        name: [
          #text(upper(data.project_name)).
        ],
    ),

    students: data.students,

    agreed_by: (
        name: data.agreed_by_name,
        position: data.agreed_by_position,
    ),

    approved_by: (
        name: data.approved_by_name,
        position: data.approved_by_position,
    ),

    university_name: [
        ПРАВИТЕЛЬСТВО РОССИЙСКОЙ ФЕДЕРАЦИИ

        ФЕДЕРАЛЬНОЕ ГОСУДАРСТВЕННОЕ АВТОНОМНОЕ

        ОБРАЗОВАТЕЛЬНОЕ УЧРЕЖДЕНИЕ ВЫСШЕГО ОБРАЗОВАНИЯ

        НАЦИОНАЛЬНЫЙ ИССЛЕДОВАТЕЛЬСКИЙ УНИВЕРСИТЕТ

        "ВЫСШАЯ ШКОЛА ЭКОНОМИКИ"
    ],

    faculty_name: [
        Факультет компьютерных наук
    ],

    edu_program_name: [
        Образовательная программа "Программная инженерия"
    ],

    year: datetime.today().year(),
)
