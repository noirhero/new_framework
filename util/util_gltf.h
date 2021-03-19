// Copyright 2018-2021 TAP, Inc. All Rights Reserved.

#pragma once

namespace Command {
    class Pool;
}

namespace Data {
    struct Model;
}

namespace GLTF {
    Data::Model& Get(std::string&& fileName, Command::Pool& cmdPool);
    void         Clear();
}
